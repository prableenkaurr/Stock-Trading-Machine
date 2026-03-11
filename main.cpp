//THIS IS A SAMPLE FOR MAIN 
//REPLACE THIS LATER TO WORK WITH FRONTEND AND WEBSITE
//AUTHOR: Jaden Palomino

#include "engine/MatchingEngine.hpp"
#include "web/WebServer.hpp"

int main() {
 
    MatchingEngine engine;
 
    web::WebServer server(engine, "web_ui");
    // Blocking: open the printed URL in your browser.
    server.listen("127.0.0.1", 8080);

    return 0;

}
