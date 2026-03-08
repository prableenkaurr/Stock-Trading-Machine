// Author: Prableen Kakar
// File: OrderBook.hpp
// This file defines the OrderBook class, which is the core data structure
// of the trading system. There is one OrderBook per stock ticker, and it
// keeps track of all the resting buy and sell orders for that stock.
// When a new order comes in, the book tries to match it against existing
// orders on the opposite side. If it can't fully fill, the remainder sits
// on the book until something matches it later.

#pragma once
#include <map>
#include <queue>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include "../models/Order.hpp"
#include "../models/Trade.hpp"

class OrderBook {
public:

    // Sets up a new empty order book for the given stock ticker.
    explicit OrderBook(const std::string& ticker);

    // Cleans up all orders still sitting on the book when the program ends.
    ~OrderBook();

    // Takes a limit order and tries to match it against the opposite side.
    // Any unmatched quantity gets added to the book. Returns a list of
    // trades that happened as a result (could be empty, one, or many
    // if the order filled across multiple price levels).
    std::vector<Trade> addLimitOrder(Order* order);

    // Takes a market order and fills it immediately at the best available
    // price. If there are not enough orders to fill it completely,
    // the leftover quantity is discarded. Market orders never sit on the book.
    std::vector<Trade> addMarketOrder(Order* order);

    // Looks up an order by its ID and marks it as cancelled. Returns true
    // if the order was found and cancelled, false if the ID does not exist
    // or was already cancelled.
    bool cancelOrder(int orderId);

    // Returns the spread, which is the difference between the best ask
    // price and the best bid price, in cents. Returns -1 if there are not
    // orders on both sides yet.
    long getSpread() const;

    // Returns the highest price any buyer is currently willing to pay,
    // in cents. Returns -1 if there are no bids on the book.
    long bestBid() const;

    // Returns the lowest price any seller is currently willing to accept,
    // in cents. Returns -1 if there are no asks on the book.
    long bestAsk() const;

    // Returns how many buy orders are currently sitting on the book.
    int bidCount() const;

    // Returns how many sell orders are currently sitting on the book.
    int askCount() const;

    // Returns the ticker symbol this book is managing.
    const std::string& getTicker() const { return ticker_; }

    // Prints a view of the top N price levels on each side of the book,
    // along with the current spread.
    void display(int levels = 5) const;

private:

    // The stock ticker this book belongs to, e.g. "AAPL".
    std::string ticker_;

    // All the resting buy orders, sorted from highest price to lowest.
    // This way the best bid (highest price) is always at the front.
    // Each price level has a queue so orders at the same price get
    // filled in the order they arrived (FIFO).
    std::map<long, std::queue<Order*>, std::greater<long>> bids_;

    // All the resting sell orders, sorted from lowest price to highest.
    // The best ask (lowest price) is always at the front.
    std::map<long, std::queue<Order*>, std::less<long>> asks_;

    // A lookup table from order ID to the order pointer. We use this
    // so cancellations can find the right order in O(1) time instead
    // of scanning through the whole book.
    std::unordered_map<int, Order*> orderIndex_;

    // Running count of how many buy orders are sitting on the book.
    int bidCount_;

    // Running count of how many sell orders are sitting on the book.
    int askCount_;

    // Internal helper that does the actual matching logic. It goes through
    // the opposite side of the book and fills the incoming order as much
    // as possible, and returns a list of trades that were generated.
    std::vector<Trade> match(Order* incoming);


};
