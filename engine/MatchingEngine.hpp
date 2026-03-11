// declares how trading will be executed and how orders will be matched to create trades
//Author: Jaden Palomino
//File: MatchingEngine.hpp

#pragma once
#include <unordered_map>
#include <string>
#include "../orderbook/OrderBook.hpp"
#include "../log/ExecutionLog.hpp"
#include "../models/Order.hpp"

class MatchingEngine {
public:
    //Constructor and Destructor
    MatchingEngine();
    ~MatchingEngine();

    //Process any incoming order
    void processOrder(Order* order);

    //Cancel an order by ID
    bool cancelOrder(int orderId);

    //Show order book for a ticker
    void displayBook(const std::string& ticker, int levels = 5) const;

    //Show all executed trades
    void displayTrades() const;

    // Non-printing accessors for programmatic UIs.
    const std::vector<Trade>& trades() const { return log_.trades(); }
    int tradeCount() const { return log_.size(); }

    // Returns true if a book exists for ticker and fills snapshot.
    bool getBookSnapshot(const std::string& ticker, OrderBook::BookSnapshot& out, int levels = 5) const;

    struct TickerStats {
        std::string ticker;
        int bids;
        int asks;
    };

    // Returns bid/ask counts for every ticker that has an order book.
    std::vector<TickerStats> getStats() const;

private:
    //All order books indexed by ticker
    std::unordered_map<std::string, OrderBook*> books_;

    //Execution Log
    ExecutionLog log_;

    //Maps order ID-> ticker in order to know which book to cancel from
    std::unordered_map<int, std::string> orderToTicker_;

    //Get or create an order book
    OrderBook* getBook(const std::string& ticker);

};