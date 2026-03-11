// stores trade history 
//Author: Jaden Palomino
//File: ExecutionLog.cpp
#pragma once
#include <vector>
#include <string>
#include "../models/Trade.hpp"

class ExecutionLog {
public:
    //Add a single trade to History
    void recordTrade(const Trade& trade);
    
    //Add multiple trades to History
    //Useful for when match() returns several
    void recordTrades(const std::vector<Trade>& trades);

    //Prints all trades
    void display() const;

    //Total number of trades executed
    int size() const;

    // Read-only access to all recorded trades (in-memory only).
    const std::vector<Trade>& trades() const { return trades_; }

private:
    //Stores all executed trades
    std::vector<Trade> trades_;
};