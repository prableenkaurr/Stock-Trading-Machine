//THIS IS A SAMPLE FOR MAIN 
//REPLACE THIS LATER TO WORK WITH FRONTEND AND WEBSITE
//AUTHOR: Jaden Palomino
//
// This version of main wires the MatchingEngine into a small HTTP server
// so that a browser-based UI can drive the trading system instead of the
// original console-only interface.

#include "engine/MatchingEngine.hpp"
#include "web/WebServer.hpp"
#include <cstdlib>
#include <string>

int main() {

    MatchingEngine engine;

    const char* portEnv = std::getenv("PORT");
    int port = portEnv ? std::stoi(portEnv) : 8080;

    web::WebServer server(engine, "web_ui");
    // Blocking: open the printed URL in your browser.
    server.listen("0.0.0.0", port);

    return 0;

}
