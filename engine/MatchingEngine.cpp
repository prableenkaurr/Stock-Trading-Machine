// handles the trading matching logic
//Author: Jaden Palomino
//File: MatchingEngine.cpp

#include "MatchingEngine.hpp"
#include <iostream>

MatchingEngine::MatchingEngine() {}

MatchingEngine::~MatchingEngine() {
    for (auto& [ticker, book] : books_) {
        delete book;
    }
}

OrderBook* MatchingEngine::getBook(const std::string& ticker) {
    auto it = books_.find(ticker);
    if (it != books_.end()){
        return it->second;
    }

    //create new orderboo if ticker doesn't exist yet
    OrderBook* newBook = new OrderBook(ticker);
    books_[ticker] = newBook;

    return newBook;
}

void MatchingEngine::processOrder(Order* order) {
    OrderBook* book = getBook(order->ticker);
    std::vector<Trade> trades;
    if (order->type == OrderType::Limit) {
        trades = book->addLimitOrder(order);

        //Track for possible cancellation
        orderToTicker_[order->id] = order->ticker;
    }
    else if (order->type == OrderType::Market) {
        trades = book->addMarketOrder(order);
    }

    //Record the trade
    log_.recordTrades(trades);
}

bool MatchingEngine::cancelOrder(int orderId) {
    auto it = orderToTicker_.find(orderId);

    if (it == orderToTicker_.end()){
        return false;
    }

    const std::string& ticker = it->second;
    
    auto bookIt = books_.find(ticker);
    if (bookIt == books_.end()){
        return false;
    }

    bool result = bookIt->second->cancelOrder(orderId);

    if (result) {
        orderToTicker_.erase(orderId);
    }

    return result;
}

void MatchingEngine::displayBook(const std::string& ticker, int levels) const {
    auto it = books_.find(ticker);
    
    if(it == books_.end()) {
        std::cout << "No order book exists for " << ticker << "\n";
        return;
    }

    it->second->display(levels);
}

void MatchingEngine::displayTrades() const {
    log_.display();
}

bool MatchingEngine::getBookSnapshot(const std::string& ticker, OrderBook::BookSnapshot& out, int levels) const {
    auto it = books_.find(ticker);
    if (it == books_.end()) return false;
    out = it->second->snapshot(levels);
    return true;
}

std::vector<MatchingEngine::TickerStats> MatchingEngine::getStats() const {
    std::vector<TickerStats> stats;
    for (const auto& [ticker, book] : books_) {
        stats.push_back({ticker, book->bidCount(), book->askCount()});
    }
    return stats;
}