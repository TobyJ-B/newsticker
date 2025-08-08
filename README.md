# Financial News Broadcaster
A C++ application that fetches financial news and broadcasts it to connected clients in real-time.

## Dependencies
build-essential
libcpr-dev
nlohmann-json3-dev

## Installation
Get a Finnhub API key from finnhub.io and update it in getnews.cpp:
```cpp
const std::string apiKey = "your_api_key_here";
```

## Compile the programs:
```bash
g++ -o getnews getnews.cpp -lcpr -pthread
g++ -o server server.cpp -pthread
g++ -o client client.cpp -pthread
```

## Run the system:
```bash
./server
```

## Connect clients (in other terminals)
```bash
./client
```

## Future enhancments

- Add a GUI either web based or Qt
- News filtering by ticker
- User authentification


The server runs on port 54000 and broadcasts news updates every 10 seconds to all connected clients.


