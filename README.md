How to Build and run? 

Compile using: 
g++ -std=c++17 -Wall main.cpp web/*.cpp ui/*.cpp orderbook/*.cpp engine/*.cpp log/*.cpp tests/*.cpp -o trading

Run using:
./trading

Then open the printed URL in your browser (defaults to `http://127.0.0.1:8080/`).

Complexity

The system uses an ordered map to store price levels, queues to maintain order priority at each price, and a hash map to quickly locate orders by ID.

Order Insertion – O(log P)
The system finds the correct price level in the ordered map (O(log P)), then adds the order to the queue and stores it in the hash map (both O(1)).

Order Cancellation – O(m)
The order is first located using the hash map (O(1)). The system then scans the queue at that price level to find and remove the order (O(m)), where m is the number of orders at that price.

Order Matching – O(k log P)
For each trade, the system checks the best price level and removes orders from the queue (O(1)). If a price level becomes empty, it is removed from the map (O(log P)). Here k is the number of matched orders.
