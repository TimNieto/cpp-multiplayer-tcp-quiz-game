# Multiplayer TCP Quiz Game in C++  

A C++ client-server quiz game that allows up to 3 players to compete in real-time. Features include multiple-choice questions, scoring, tiebreaker rounds, and a live scoreboard.  

## Features
- TCP socket-based client-server communication
- Multithreading for concurrent player handling
- Mutex-protected shared state for thread safety
- Real-time scoreboard and scoring system
- Tiebreaker rounds for tied scores
- Simple console-based client interface  

## Technologies & Concepts
- C++17
- TCP/IP sockets (`<sys/socket.h>`, `<arpa/inet.h>`, `<netinet/in.h>`)
- Multithreading with `<thread>` and `<mutex>`
- Standard data structures (`vector`, `map`) for question and player management  

## Usage

### Server
Compile the server:
```bash
g++ server.cpp -o server -pthread

./server

g++ client.cpp -o client

./client
