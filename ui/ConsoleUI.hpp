// Author: Astha Desai
// File: ConsoleUI.hpp
// This file defines the ConsoleUI class, which is responsible for
// interacting with the user through the terminal. The UI allows users
// to place limit and market orders, cancel existing orders, and display
// the current state of the order book.
//
// The UI does not perform any matching logic itself. Instead, it creates
// Order objects and sends them to the appropriate OrderBook instance.
// Each stock ticker has its own OrderBook.

#pragma once

#include <map>
#include <string>
#include "../orderbook/OrderBook.hpp"
#include "../log/ExecutionLog.hpp"

class ConsoleUI {

public:
    // Constructor that initializes the UI and sets up the
    // internal order ID counter.
    ConsoleUI();

    // Starts the interactive loop that displays the menu and
    // waits for user input. This function runs until the user
    // chooses to exit the program.
    void run();

private:

    // A map from ticker symbol to its OrderBook instance.
    // This allows the system to support multiple stocks at once.
    // Example:
    // "AAPL" → OrderBook for Apple
    // "GOOG" → OrderBook for Google
    std::map<std::string, OrderBook*> books;

    // A simple incrementing counter used to generate unique
    // order IDs for each new order placed through the UI.
    int nextOrderId;

    //Adds the Execution Log to the UI
    ExecutionLog log;

    // Returns the OrderBook for the given ticker. If the book
    // does not exist yet, it creates a new one automatically.
    OrderBook* getOrCreateBook(const std::string& ticker);

    // Reads user input and creates a new limit order.
    // Limit orders may sit on the book if they are not fully matched.
    void placeLimitOrder();

    // Reads user input and creates a market order.
    // Market orders are executed immediately at the best available price
    // and never remain on the book.
    void placeMarketOrder();

    // Allows the user to cancel an existing order using its ID.
    void cancelOrder();

    // Displays the current state of the order book for a specific ticker.
    void displayBook();

    
};