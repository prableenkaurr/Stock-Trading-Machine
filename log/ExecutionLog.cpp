// implements trade logging
//Author: Jaden Palomino
//File: ExecutionLog.cpp

#include "ExecutionLog.hpp"
#include <iostream>
#include <iomanip>

void ExecutionLog::recordTrade(const Trade& trade) {
    trades_.push_back(trade);
}

void ExecutionLog::recordTrades(const std::vector<Trade>& trades) {
    for (const auto& t : trades) {
        recordTrade(t);
    }
}

int ExecutionLog::size() const {
    return trades_.size();
}

void ExecutionLog::display() const {
    std::cout << "\n=== EXECUTION LOG ===\n";

    if (trades_.empty()) {
        std::cout << "No Trades Executed Yet.\n";
        return;
    }
    for (const auto& t : trades_) {
        std::cout << "TRADE: Order #"
                  << t.buyOrderId
                  << " matched with Order #"
                  << t.sellOrderId
                  << " for "
                  << t.quantity
                  << " shares of "
                  << t.ticker
                  << " at $"
                  << std::fixed << std::setprecision(2)
                  << (t.price / 100.0)
                  << "\n";
    }
    //Sample Print:
    //TRADE: Order: #101 matched with Order #88 for 50 shares of <xyz> at $102.32

}