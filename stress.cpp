//Author: Jaden Palomino
//File: stress.cpp
//Description: creates a selected amount of 
//orders (10,000) that are completely random.
//any price between 140 and 160, and any quantity 
//between 1 to 100. It then prints the orders processed
//the trades executed, and the time taken. 
//run with: g++ stressTest.cpp orderbook/OrderBook.cpp log/ExecutionLog.cpp engine/MatchingEngine.cpp -o stress
// and ./stress

#include "../orderbook/OrderBook.hpp"
#include "../log/ExecutionLog.hpp"
#include <iostream>
#include <random>
#include <chrono>

int main() {

    OrderBook book("AAPL");
    ExecutionLog log;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> sideDist(0,1);
    std::uniform_int_distribution<int> priceDist(14000,16000);
    std::uniform_int_distribution<int> qtyDist(1,100);

    int nextId = 1;
    const int NUM_ORDERS = 100000;

    auto start = std::chrono::high_resolution_clock::now();

    for(int i = 0; i < NUM_ORDERS; i++) {

        OrderSide side = sideDist(rng) ? OrderSide::Buy : OrderSide::Sell;
        long price = priceDist(rng);
        int qty = qtyDist(rng);

        Order* order = new Order(
            nextId++,
            OrderType::Limit,
            side,
            "AAPL",
            price,
            qty
        );

        auto trades = book.addLimitOrder(order);

        log.recordTrades(trades);
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Orders processed: " << NUM_ORDERS << "\n";
    std::cout << "Trades executed: " << log.size() << "\n";
    std::cout << "Time taken: " << elapsed.count() << " seconds\n";

    return 0;
}