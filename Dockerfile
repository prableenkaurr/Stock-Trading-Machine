FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN g++ -std=c++17 -O2 \
    main.cpp \
    web/WebServer.cpp \
    ui/ConsoleUI.cpp \
    orderbook/OrderBook.cpp \
    engine/MatchingEngine.cpp \
    log/ExecutionLog.cpp \
    -o trading

EXPOSE 8080

CMD ["./trading"]
