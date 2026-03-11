// Author: Prableen Kakar
// File: OrderBook.cpp
// This file contains the actual implementations of all the functions
// declared in OrderBook.hpp.

#include "OrderBook.hpp"
#include <iostream>
#include <iomanip>

// Sets the ticker and starts both counts at zero since the book is empty.
OrderBook::OrderBook(const std::string& ticker)
    : ticker_(ticker), bidCount_(0), askCount_(0) {}

// When the book is destroyed, we need to free every order that is still
// sitting in the queues. We go through bids and asks separately and
// delete each order pointer we find.
OrderBook::~OrderBook() {
    for (auto& [price, q] : bids_) {
        while (!q.empty()) { delete q.front(); q.pop(); }
    }
    for (auto& [price, q] : asks_) {
        while (!q.empty()) { delete q.front(); q.pop(); }
    }
}

// Adds the order to our lookup table first so it can be cancelled later.
// Then we try to match it against the opposite side. Whatever quantity
// is left over after matching gets placed on the book at its limit price.
std::vector<Trade> OrderBook::addLimitOrder(Order* order) {
    orderIndex_[order->id] = order;

    std::vector<Trade> trades = match(order);

    if (order->quantity > 0 && !order->cancelled) {
        if (order->side == OrderSide::Buy) {
            bids_[order->price].push(order);
            ++bidCount_;
        } else {
            asks_[order->price].push(order);
            ++askCount_;
        }
    } else if (order->quantity == 0) {
        // The order was fully filled during matching so it won't sit on the book.
        // We need to free it here since no queue will hold it.
        orderIndex_.erase(order->id);
        delete order;
    }
    return trades;
}

// Market orders skip the book. We just try to fill as much as
// possible right now. If there is still quantity left after we run out
// of orders on the other side, we throw the remainder away and clean up.
std::vector<Trade> OrderBook::addMarketOrder(Order* order) {
    orderIndex_[order->id] = order;
    std::vector<Trade> trades = match(order);

    // Whether fully filled or not, market orders never sit on the book.
    // Always clean up the index entry and free the memory.
    orderIndex_.erase(order->id);
    delete order;
    return trades;
}

// We look up the order by ID in the index. If we find it and it hasn't
// already been cancelled, we mark it cancelled and adjust the count.
// The order stays in its queue but will be skipped the next time matching runs.
bool OrderBook::cancelOrder(int orderId) {
    auto it = orderIndex_.find(orderId);
    if (it == orderIndex_.end()) return false;

    Order* o = it->second; // Gets the order pointer from the index.
    if (o->cancelled) return false;

    o->cancelled = true;

    if (o->side == OrderSide::Buy)  --bidCount_;
    else                             --askCount_;

    orderIndex_.erase(it); //Removes from the lookup table, not from the queue
    return true;
}

// We just subtract the best bid from the best ask. If either side of
// the book is empty, there is no spread so we return -1.
long OrderBook::getSpread() const {
    long ba = bestAsk();
    long bb = bestBid();
    if (ba == -1 || bb == -1) return -1;
    return ba - bb;
}

// The best bid is the highest price in the bids map, which is always
// the first entry since the map is sorted highest to lowest.
// We skip over any levels that only have cancelled orders sitting in them.
long OrderBook::bestBid() const {
    for (auto& [price, q] : bids_) {
        if (!q.empty()) return price;
    }
    return -1;
}

// The best ask is the lowest price in the asks map, which is always
// the first entry since the map is sorted lowest to highest.
long OrderBook::bestAsk() const {
    for (auto& [price, q] : asks_) {
        if (!q.empty()) return price;
    }
    return -1;
}

int OrderBook::bidCount() const { return bidCount_; }
int OrderBook::askCount() const { return askCount_; }

// Prints the book in the standard format: asks on top (highest to lowest),
// spread in the middle, bids on the bottom (highest to lowest).
void OrderBook::display(int levels) const {
    std::cout << "\n Order Book: " << ticker_ << " \n";
    std::cout << std::setw(12) << "Ask Quantity" << "  " << std::setw(10) << "Price" << "\n";

    // Collect the ask levels we want to show. We total up all the
    // non-cancelled quantity at each price level before printing.
    std::vector<std::pair<long, int>> askLevels;
    for (auto& [price, q] : asks_) {
        int total = 0;
        std::queue<Order*> tmp = q; // copy the queue so we can iterate through it without modifying the real order book.
        while (!tmp.empty()) {
            Order* o = tmp.front(); tmp.pop();
            if (!o->cancelled) total += o->quantity; // to show the total quantity available at this price level, not individual orders.
        }
        if (total > 0) askLevels.push_back({price, total});
        if ((int)askLevels.size() >= levels) break;
    }

    // Print asks from highest to lowest so the best ask is closest to
    // the spread line in the middle.
    for (auto it = askLevels.rbegin(); it != askLevels.rend(); ++it) {
        std::cout << std::setw(12) << it->second
                  << "  $" << std::setw(8) << std::fixed << std::setprecision(2)
                  << it->first / 100.0 << "\n";
    }

    long spread = getSpread();
    if (spread >= 0) {
        std::cout << " spread: $" << std::fixed << std::setprecision(2)
                  << spread / 100.0 << " \n";
    } else {
        std::cout << " no spread \n";
    }

    // Print the top bid levels from highest to lowest.
    int shown = 0;
    for (auto& [price, q] : bids_) {
        if (shown >= levels) break;
        int total = 0;
        std::queue<Order*> tmp = q; // copy the queue so we can iterate through it without modifying the real order book.
        while (!tmp.empty()) {
            Order* o = tmp.front(); tmp.pop();
            if (!o->cancelled) total += o->quantity; // to show the total quantity available at this price level, not individual orders.
        }
        if (total > 0) {
            std::cout << std::setw(12) << total
                      << "  $" << std::setw(8) << std::fixed << std::setprecision(2)
                      << price / 100.0 << "\n";
            ++shown;
        }
    }
    std::cout << std::setw(12) << "Bid quantity" << "  " << std::setw(10) << "Price" << "\n";
}

OrderBook::BookSnapshot OrderBook::snapshot(int levels) const {
    BookSnapshot snap;
    snap.ticker = ticker_;
    snap.spreadCents = getSpread();

    // asks_ is sorted ascending by price (best ask first)
    for (auto& [price, q] : asks_) {
        if ((int)snap.asks.size() >= levels) break;
        int total = 0;
        std::queue<Order*> tmp = q;
        while (!tmp.empty()) {
            Order* o = tmp.front(); tmp.pop();
            if (!o->cancelled) total += o->quantity;
        }
        if (total > 0) snap.asks.push_back({price, total});
    }

    // bids_ is sorted descending by price (best bid first)
    for (auto& [price, q] : bids_) {
        if ((int)snap.bids.size() >= levels) break;
        int total = 0;
        std::queue<Order*> tmp = q;
        while (!tmp.empty()) {
            Order* o = tmp.front(); tmp.pop();
            if (!o->cancelled) total += o->quantity;
        }
        if (total > 0) snap.bids.push_back({price, total});
    }

    return snap;
}

// This is where the actual matching happens. We look at which side the
// incoming order is on and walk through the opposite side of the book.
// For each resting order we find, we calculate how many shares can be
// filled right now, record a trade, and update both quantities.
// We stop when the incoming order is fully filled or there are no more
// orders on the other side that meet the price requirement.
std::vector<Trade> OrderBook::match(Order* incoming) {
    std::vector<Trade> trades;
    bool isBuy = (incoming->side == OrderSide::Buy);

    while (incoming->quantity > 0) {
        if (isBuy) {
            if (asks_.empty()) break;
            auto it = asks_.begin();
            long bestPrice = it->first;

            // For limit orders, stop if the best available ask is higher
            // than what the buyer is willing to pay.
            if (incoming->type == OrderType::Limit && incoming->price < bestPrice) break;

            auto& levelQueue = it->second;
            while (!levelQueue.empty() && incoming->quantity > 0) {
                Order* resting = levelQueue.front();

                // Skip any orders that were cancelled while sitting in the queue.
                // We also delete the memory here since no one else will free it.
                if (resting->cancelled) {
                    levelQueue.pop();
                    delete resting;
                    continue;
                }

                int filled = std::min(incoming->quantity, resting->quantity);

                Trade t;
                t.buyOrderId  = incoming->id;
                t.sellOrderId = resting->id;
                t.ticker      = ticker_;
                t.price       = bestPrice;
                t.quantity    = filled;
                trades.push_back(t);

                incoming->quantity -= filled;
                resting->quantity  -= filled;

                // If the resting order is fully filled, remove it from
                // the queue and clean it up from the index and memory.
                if (resting->quantity == 0) {
                    levelQueue.pop();
                    orderIndex_.erase(resting->id);
                    delete resting;
                    --askCount_;
                }
            }

            // If that price level is now empty, remove it from the map.
            if (levelQueue.empty()) asks_.erase(it);

        } else {
            if (bids_.empty()) break;
            auto it = bids_.begin();
            long bestPrice = it->first;

            // For limit orders, stop if the best available bid is lower
            // than what the seller is willing to accept.
            if (incoming->type == OrderType::Limit && incoming->price > bestPrice) {
                std::cout << "Error: bid is lower than price, please enter a higher price next time.\n";
                break;
            }

            auto& levelQueue = it->second;
            while (!levelQueue.empty() && incoming->quantity > 0) {
                Order* resting = levelQueue.front();

                if (resting->cancelled) {
                    levelQueue.pop();
                    delete resting;
                    continue;
                }

                int filled = std::min(incoming->quantity, resting->quantity);

                Trade t;
                t.buyOrderId  = resting->id;
                t.sellOrderId = incoming->id;
                t.ticker      = ticker_;
                t.price       = bestPrice;
                t.quantity    = filled;
                trades.push_back(t);

                incoming->quantity -= filled;
                resting->quantity  -= filled;

                if (resting->quantity == 0) {
                    levelQueue.pop();
                    orderIndex_.erase(resting->id);
                    delete resting;
                    --bidCount_;
                }
            }

            if (levelQueue.empty()) bids_.erase(it);
        }
    }
    return trades;
}
