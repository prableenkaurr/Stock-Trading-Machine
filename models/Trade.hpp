// Author: Prableen Kakar
// File: Trade.hpp
// This file defines what a completed trade looks like.
// Whenever a buy order and a sell order match, we create one of these
// and add it to the execution log so there's a record of what happened.

#pragma once
#include <string>

struct Trade {

    // The ID of the buy order that was involved in this trade.
    int buyOrderId;

    // The ID of the sell order that was involved in this trade.
    int sellOrderId;

    // The stock symbol that was traded.
    std::string ticker;

    // The price the trade actually happened at, in cents.
    // For example $150.25 is stored as 15025.
    long price;

    // The number of shares that actually changed hands in this trade.
    // This can be less than the original order size if it was a partial fill.
    int quantity;
};
