// Author: Prableen Kakar
// File: Order.hpp
// This file defines what an order looks like in the trading system.
// Every time a user wants to buy or sell a stock, we create one of these.

#pragma once
#include <string>
#include <chrono>

// These are the three types of orders a user can place.
// A LIMIT order means "buy/sell at this price or better."
// A MARKET order means "buy/sell right now at whatever price is available."
// A CANCEL order means "remove one of my existing orders from the book."
enum class OrderType { Limit, Market, Cancel };

// This just tracks which side of the trade the order is on.
// Buy means the user wants to purchase shares, Sell means they want to sell.
enum class OrderSide { Buy, Sell };

struct Order {

    // A unique number we assign to every order so we can look it up later.
    int id;

    // Whether this is a limit, market, or cancel order.
    OrderType type;

    // Whether the user is trying to buy or sell.
    OrderSide side;

    // The stock symbol this order is for, like "AVGO" or "GOOG".
    std::string ticker;

    // The price the user is willing to trade at, stored in cents to avoid
    // floating point rounding errors. So $150.25 is stored as 15025.
    // This field is ignored for market orders since they take any price.
    long price;

    // How many shares are left to fill. This starts equal to origQty and
    // goes down as the order gets matched (partial fills are allowed).
    int quantity;

    // The original number of shares the user asked for. We keep this
    // around for record-keeping even after partial fills.
    int origQty;

    // The exact time this order was placed, in nanoseconds. We use this
    // to break ties when two orders have the same price. Whoever placed
    // their order first gets matched first (FIFO).
    long long timestamp;

    // Flipped to true if the user cancels this order. We do not immediately
    // remove it from the queue, we just mark it and skip it during matching 
    // for efficiency.
    bool cancelled;

    // This is the constructor. It sets up a new order with all the info
    // we need, and automatically records the current time as the timestamp.
    Order(int id, OrderType type, OrderSide side, const std::string& ticker,
          long price, int quantity)
        : id(id), type(type), side(side), ticker(ticker),
          price(price), quantity(quantity), origQty(quantity),
          cancelled(false)
    {
        timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
    }
};
