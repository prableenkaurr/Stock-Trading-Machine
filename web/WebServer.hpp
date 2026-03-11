#pragma once

#include <atomic>
#include <mutex>
#include <string>

#include "../engine/MatchingEngine.hpp"

namespace web {

class WebServer {
public:
    WebServer(MatchingEngine& engine, std::string staticDir);

    // Blocking call that starts listening.
    // Returns true if the server started successfully.
    bool listen(const std::string& host, int port);

private:
    MatchingEngine& engine_;
    std::string staticDir_;
    std::mutex mu_;
    std::atomic<int> nextOrderId_{1};

    void registerRoutes();

    // cpp-httplib server is defined in the .cpp to keep headers light.
    struct Impl;
    Impl* impl_;
};

} // namespace web

