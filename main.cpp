//THIS IS A SAMPLE FOR MAIN 
//REPLACE THIS LATER TO WORK WITH FRONTEND AND WEBSITE
//AUTHOR: Jaden Palomino

#include "engine/MatchingEngine.hpp"
#include "ui/ConsoleUI.hpp"

int main() {
 
    /*
    MatchingEngine engine;

    Order* o1 = new Order(1, OrderType::Limit, OrderSide::Buy, "IREN", 37.43, 50);
    Order* o2 = new Order(2, OrderType::Limit, OrderSide::Sell, "IREN", 39.00, 30);

    engine.processOrder(o1);
    engine.processOrder(o2);

    engine.displayBook("IREN");
    engine.displayTrades();
    */

    ConsoleUI ui;
    ui.run();

    return 0;

}