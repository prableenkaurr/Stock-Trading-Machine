// File: WebServer.cpp
// Author: Margarita Schemel
// This file contains the implementation of the WebServer class declared
// in WebServer.hpp. It wires the MatchingEngine into a minimal HTTP server
// so that a browser-based UI can place orders, cancel them, and inspect
// the live order books and trade history.

#include "WebServer.hpp"

#include <cctype>
#include <iostream>
#include <optional>

#include "../models/Order.hpp"
#include "../third_party/httplib.h"
#include "../third_party/json.hpp"

namespace web {

using json = nlohmann::json;

// Thin wrapper around the underlying cpp-httplib server. Keeping this in
// an inner struct lets us hide the third_party header from WebServer.hpp.
struct WebServer::Impl {
    httplib::Server svr;
};

// Normalizes a ticker string to upper case so that Aapl, aapl and AAPL
// all refer to the same order book internally.
static std::string toUpper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

// Parses a user-provided side string into the OrderSide enum.
static std::optional<OrderSide> parseSide(const std::string& s) {
    if (s == "buy") return OrderSide::Buy;
    if (s == "sell") return OrderSide::Sell;
    return std::nullopt;
}

// Parses a user-provided type string into the OrderType enum.
static std::optional<OrderType> parseType(const std::string& s) {
    if (s == "limit") return OrderType::Limit;
    if (s == "market") return OrderType::Market;
    return std::nullopt;
}

// Converts a Trade struct into a JSON object that is easy for the
// JavaScript frontend to consume.
static json tradeToJson(const Trade& t) {
    return json{
        {"buyOrderId", t.buyOrderId},
        {"sellOrderId", t.sellOrderId},
        {"ticker", t.ticker},
        {"price", t.price / 100.0},
        {"quantity", t.quantity},
    };
}

// Converts a compact BookSnapshot into JSON arrays of price levels
// so that the UI can render the order book.
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

// We keep a reference to the shared MatchingEngine and the name of the
// directory that holds the static web assets. The actual HTTP routes
// are registered immediately in the constructor.
WebServer::WebServer(MatchingEngine& engine, std::string staticDir)
    : engine_(engine), staticDir_(std::move(staticDir)), impl_(new Impl()) {
    registerRoutes();
}

bool WebServer::listen(const std::string& host, int port) {
    // Print the URL once so the user knows where to point their browser.
    std::cout << "Web UI: http://" << host << ":" << port << "/" << std::endl;
    return impl_->svr.listen(host, port);
}

void WebServer::registerRoutes() {
    // Serve everything under web_ui/ directly at the root path. The main
    // entry point is index.html, which calls back into the JSON API below.
    impl_->svr.set_mount_point("/", staticDir_.c_str());

    // Simple health check so the frontend can show “Connected” vs error.
    impl_->svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(json{{"ok", true}}.dump(), "application/json");
    });

    // Places a new limit or market order and returns the assigned order ID
    // plus any trades that were generated immediately.
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

    // Cancels an existing order by ID if it is still active on one of the
    // books. The response indicates whether the cancellation succeeded.
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

    // Returns a compact snapshot of the top N price levels for a given
    // ticker so the UI can render the live order book without scraping
    // console output.
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

    // Returns basic stats (bid/ask counts) per ticker across all books so
    // the UI can show how many resting orders exist in the system.
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

    // Returns every trade ever recorded during this process lifetime so
    // the UI can render the full execution log in one call.
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

