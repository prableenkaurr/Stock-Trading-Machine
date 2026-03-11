#include "WebServer.hpp"

#include <cctype>
#include <iostream>
#include <optional>

#include "../models/Order.hpp"
#include "../third_party/httplib.h"
#include "../third_party/json.hpp"

namespace web {

using json = nlohmann::json;

struct WebServer::Impl {
    httplib::Server svr;
};

static std::string toUpper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

static std::optional<OrderSide> parseSide(const std::string& s) {
    if (s == "buy") return OrderSide::Buy;
    if (s == "sell") return OrderSide::Sell;
    return std::nullopt;
}

static std::optional<OrderType> parseType(const std::string& s) {
    if (s == "limit") return OrderType::Limit;
    if (s == "market") return OrderType::Market;
    return std::nullopt;
}

static json tradeToJson(const Trade& t) {
    return json{
        {"buyOrderId", t.buyOrderId},
        {"sellOrderId", t.sellOrderId},
        {"ticker", t.ticker},
        {"price", t.price / 100.0},
        {"quantity", t.quantity},
    };
}

static json bookSnapshotToJson(const OrderBook::BookSnapshot& s) {
    json asks = json::array();
    for (const auto& lvl : s.asks) {
        asks.push_back({{"price", lvl.priceCents / 100.0}, {"qty", lvl.quantity}});
    }
    json bids = json::array();
    for (const auto& lvl : s.bids) {
        bids.push_back({{"price", lvl.priceCents / 100.0}, {"qty", lvl.quantity}});
    }

    json out{
        {"ticker", s.ticker},
        {"asks", asks},
        {"bids", bids},
    };
    if (s.spreadCents >= 0) out["spread"] = s.spreadCents / 100.0;
    else out["spread"] = nullptr;
    return out;
}

WebServer::WebServer(MatchingEngine& engine, std::string staticDir)
    : engine_(engine), staticDir_(std::move(staticDir)), impl_(new Impl()) {
    registerRoutes();
}

bool WebServer::listen(const std::string& host, int port) {
    std::cout << "Web UI: http://" << host << ":" << port << "/" << std::endl;
    return impl_->svr.listen(host, port);
}

void WebServer::registerRoutes() {
    // Static UI
    impl_->svr.set_mount_point("/", staticDir_.c_str());

    // Health
    impl_->svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(json{{"ok", true}}.dump(), "application/json");
    });

    // Place order
    impl_->svr.Post("/api/orders", [this](const httplib::Request& req, httplib::Response& res) {
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", "Invalid JSON"}}.dump(), "application/json");
            return;
        }

        const std::string tickerRaw = body.value("ticker", "");
        const std::string sideRaw = body.value("side", "");
        const std::string typeRaw = body.value("type", "");
        const int quantity = body.value("quantity", 0);

        if (tickerRaw.empty() || quantity <= 0) {
            res.status = 400;
            res.set_content(json{{"error", "ticker and quantity are required"}}.dump(), "application/json");
            return;
        }

        auto sideOpt = parseSide(sideRaw);
        auto typeOpt = parseType(typeRaw);
        if (!sideOpt || !typeOpt) {
            res.status = 400;
            res.set_content(json{{"error", "side must be buy/sell and type must be limit/market"}}.dump(), "application/json");
            return;
        }

        long priceCents = 0;
        if (*typeOpt == OrderType::Limit) {
            if (!body.contains("price")) {
                res.status = 400;
                res.set_content(json{{"error", "price is required for limit orders"}}.dump(), "application/json");
                return;
            }
            priceCents = body.value("price", 0L);
            if (priceCents <= 0) {
                res.status = 400;
                res.set_content(json{{"error", "price must be > 0"}}.dump(), "application/json");
                return;
            }
        }

        const std::string ticker = toUpper(tickerRaw);

        json response;
        {
            std::lock_guard<std::mutex> lk(mu_);

            const int orderId = nextOrderId_.fetch_add(1);
            const size_t before = static_cast<size_t>(engine_.tradeCount());

            Order* order = new Order(
                orderId,
                *typeOpt,
                *sideOpt,
                ticker,
                priceCents,
                quantity
            );

            engine_.processOrder(order);

            const auto& allTrades = engine_.trades();
            json newTrades = json::array();
            for (size_t i = before; i < allTrades.size(); ++i) {
                newTrades.push_back(tradeToJson(allTrades[i]));
            }

            response = json{
                {"orderId", orderId},
                {"trades", newTrades},
            };
        }

        res.set_content(response.dump(), "application/json");
    });

    // Cancel order
    impl_->svr.Post("/api/cancel", [this](const httplib::Request& req, httplib::Response& res) {
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", "Invalid JSON"}}.dump(), "application/json");
            return;
        }

        const int orderId = body.value("orderId", 0);
        if (orderId <= 0) {
            res.status = 400;
            res.set_content(json{{"error", "orderId is required"}}.dump(), "application/json");
            return;
        }

        bool cancelled = false;
        {
            std::lock_guard<std::mutex> lk(mu_);
            cancelled = engine_.cancelOrder(orderId);
        }

        res.set_content(json{{"cancelled", cancelled}}.dump(), "application/json");
    });

    // Book snapshot
    impl_->svr.Get("/api/book", [this](const httplib::Request& req, httplib::Response& res) {
        const std::string tickerRaw = req.get_param_value("ticker");
        if (tickerRaw.empty()) {
            res.status = 400;
            res.set_content(json{{"error", "ticker query param is required"}}.dump(), "application/json");
            return;
        }

        int levels = 5;
        if (req.has_param("levels")) {
            try {
                levels = std::max(1, std::stoi(req.get_param_value("levels")));
            } catch (...) {
                levels = 5;
            }
        }

        const std::string ticker = toUpper(tickerRaw);
        OrderBook::BookSnapshot snap;
        bool ok = false;
        {
            std::lock_guard<std::mutex> lk(mu_);
            ok = engine_.getBookSnapshot(ticker, snap, levels);
        }

        if (!ok) {
            res.status = 404;
            res.set_content(json{{"error", "No order book exists for ticker"}}.dump(), "application/json");
            return;
        }

        res.set_content(bookSnapshotToJson(snap).dump(), "application/json");
    });

    // Admin stats: bid/ask counts per ticker and totals
    impl_->svr.Get("/api/stats", [this](const httplib::Request&, httplib::Response& res) {
        json tickers = json::array();
        int totalBids = 0, totalAsks = 0;
        {
            std::lock_guard<std::mutex> lk(mu_);
            for (const auto& s : engine_.getStats()) {
                tickers.push_back({{"ticker", s.ticker}, {"bids", s.bids}, {"asks", s.asks}});
                totalBids += s.bids;
                totalAsks += s.asks;
            }
        }
        res.set_content(json{{"tickers", tickers}, {"totalBids", totalBids}, {"totalAsks", totalAsks}}.dump(), "application/json");
    });

    // All trades
    impl_->svr.Get("/api/trades", [this](const httplib::Request&, httplib::Response& res) {
        json arr = json::array();
        {
            std::lock_guard<std::mutex> lk(mu_);
            for (const auto& t : engine_.trades()) arr.push_back(tradeToJson(t));
        }
        res.set_content(json{{"trades", arr}}.dump(), "application/json");
    });
}

} // namespace web

