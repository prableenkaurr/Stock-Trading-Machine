// Author: Astha Desai
// File: ConsoleUI.cpp
// This file contains the implementation of the ConsoleUI class.
// The UI presents a simple command-line menu where the user can
// place orders, cancel orders, and inspect the order book.
//
// The UI converts user input into Order objects and passes them
// to the appropriate OrderBook instance.

#include "ConsoleUI.hpp"
#include <iostream>

// Constructor initializes the order ID counter.
// Each new order receives a unique ID that increments sequentially.
ConsoleUI::ConsoleUI() {
    nextOrderId = 1;
}

// Returns the OrderBook associated with a given ticker symbol.
// If a book does not exist yet, we create one automatically.
// This allows the system to support multiple stocks dynamically.
OrderBook* ConsoleUI::getOrCreateBook(const std::string& ticker) {

    if (books.find(ticker) == books.end()) {
        books[ticker] = new OrderBook(ticker);
    } else {
        std::cout << "Found the book for this ticker: " << ticker << std::endl;
    }

    return books[ticker];
}

// Main interactive loop of the UI.
// Displays the menu and waits for the user to choose an option.
// The loop continues until the user selects Exit.
void ConsoleUI::run() {

    while (true) {

        std::cout << "\n===== Stock Trading Machine =====\n";
        std::cout << "1. Place Limit Order\n";
        std::cout << "2. Place Market Order\n";
        std::cout << "3. Cancel Order\n";
        std::cout << "4. Display Order Book\n";
        std::cout << "5. Exit\n";
        std::cout << "Select option: ";

        int choice;
        std::cin >> choice;

        switch (choice) {

            case 1:
                placeLimitOrder();
                break;

            case 2:
                placeMarketOrder();
                break;

            case 3:
                cancelOrder();
                break;

            case 4:
                displayBook();
                break;

            case 5:
                return;

            default:
                std::cout << "Invalid option\n";
        }
        log.display();
    }
}

// Prompts the user for order details and submits a new limit order.
// Limit orders specify a price and may remain on the book if they
// cannot be matched immediately.
void ConsoleUI::placeLimitOrder() {
    std::cout << "Place Limit Order function: \n";

    std::string ticker;
    std::string sideInput;
    double priceInput;
    int quantity;

    std::cout << "Ticker: ";
    std::cin >> ticker;

    std::cout << "Side (buy/sell): ";
    std::cin >> sideInput;

    std::cout << "Price: ";
    std::cin >> priceInput;

    std::cout << "Quantity: ";
    std::cin >> quantity;

    // Convert text input into the OrderSide enum
    OrderSide side;

    if (sideInput == "buy")
        side = OrderSide::Buy;
    else
        side = OrderSide::Sell;

    // Convert price from dollars to cents (long integer)
    long price = static_cast<long>(priceInput * 100);

    // Create the new order
    const int orderId = nextOrderId++;
    Order* order = new Order(
        orderId,
        OrderType::Limit,
        side,
        ticker,
        price,
        quantity
    );

    std::cout << "Order placed. ID: " << orderId << "\n";

    // Send the order to the appropriate order book
    auto trades = getOrCreateBook(ticker)->addLimitOrder(order);

    //logs the trade
    log.recordTrades(trades);

    // Print any trades generated during matching
    for (const auto& trade : trades) {

        std::cout << "TRADE: Buy#" << trade.buyOrderId
                  << " Sell#" << trade.sellOrderId
                  << " Qty:" << trade.quantity
                  << " Price:$" << trade.price / 100.0
                  << "\n";
    }
}

// Prompts the user for order details and submits a market order.
// Market orders execute immediately at the best available price.
// Any unfilled quantity is discarded and does not remain on the book.
void ConsoleUI::placeMarketOrder() {
    std::cout << "Place Market Order function: \n"; 

    std::string ticker;
    std::string sideInput;
    int quantity;

    std::cout << "Ticker: ";
    std::cin >> ticker;

    std::cout << "Side (buy/sell): ";
    std::cin >> sideInput;

    std::cout << "Quantity: ";
    std::cin >> quantity;

    // Convert string input into the OrderSide enum
    OrderSide side;

    if (sideInput == "buy")
        side = OrderSide::Buy;
    else
        side = OrderSide::Sell;

    // Market orders do not use a price, so we pass 0
    const int orderId = nextOrderId++;
    Order* order = new Order(
        orderId,
        OrderType::Market,
        side,
        ticker,
        0,
        quantity
    );

    std::cout << "Order placed. ID: " << orderId << "\n";

    // Send the market order to the order book
    auto trades = getOrCreateBook(ticker)->addMarketOrder(order);

    //logs the trades
    log.recordTrades(trades);
    
    // Display any trades that occurred
    for (const auto& trade : trades) {

        std::cout << "TRADE: Buy#" << trade.buyOrderId
                  << " Sell#" << trade.sellOrderId
                  << " Qty:" << trade.quantity
                  << " Price:$" << trade.price / 100.0
                  << "\n";
    }
}

// Allows the user to cancel an existing order using its ID.
// The order book marks the order as cancelled and it will be skipped
// during future matching operations.
void ConsoleUI::cancelOrder() {
    std::cout << "Cancel Order function: \n";
        
    std::string ticker;
    int orderId;

    std::cout << "Ticker: ";
    std::cin >> ticker;

    std::cout << "Order ID to cancel: ";
    std::cin >> orderId;

    if (books.find(ticker) == books.end()) {
        std::cout << "No such ticker\n";
        return;
    }

    bool success = books[ticker]->cancelOrder(orderId);

    if (success)
        std::cout << "Order cancelled\n";
    else
        std::cout << "Order not found\n";
}

// Displays the current state of the order book for a given ticker.
// The display() function inside OrderBook prints the top price levels,
// along with the spread between the best bid and best ask.
void ConsoleUI::displayBook() {
    std::cout << "Display orders function \n"; 

    std::string ticker;

    std::cout << "Ticker: ";
    std::cin >> ticker;

    if (books.find(ticker) == books.end()) {
        std::cout << "No orders for this ticker\n";
        return;
    }

    books[ticker]->display();
}