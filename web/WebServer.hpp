// File: WebServer.hpp
// Author: Margarita Schemel
// This header declares the WebServer class, which exposes the MatchingEngine
// over a small HTTP+JSON API and serves the static HTML/CSS/JS frontend.
// The implementation lives in WebServer.cpp and uses the single-header
// cpp-httplib and nlohmann::json libraries vendored under third_party/.

#pragma once

#include <atomic>
#include <mutex>
#include <string>

#include "../engine/MatchingEngine.hpp"

namespace web {

class WebServer {
public:
    // Constructs a server that uses the given MatchingEngine instance for all
    // trading operations and serves static files from staticDir.
    WebServer(MatchingEngine& engine, std::string staticDir);

    // Blocking call that starts listening on the given host/port.
    // Returns true if the server started successfully.
    bool listen(const std::string& host, int port);

private:
    // Shared MatchingEngine instance backing all API requests.
    MatchingEngine& engine_;

    // Directory containing index.html, styles.css, app.js, etc.
    std::string staticDir_;

    // Guards access to the engine so requests cannot mutate it concurrently.
    std::mutex mu_;

    // Simple in-memory source of unique order IDs for web-submitted orders.
    std::atomic<int> nextOrderId_{1};

    // Registers all HTTP routes on the underlying httplib::Server.
    void registerRoutes();

    // cpp-httplib server is defined in the .cpp to keep this header light.
    struct Impl;
    Impl* impl_;
};

} // namespace web

