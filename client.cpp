#include <cctype>
#include <cstring>
#include <iostream>
#include <string>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr int PORT = 12345;
constexpr int BUFFER_SIZE = 1024;

std::string get_required_input(const std::string& prompt) {
    std::string input;

    while (true) {
        std::getline(std::cin, input);

        if (!input.empty()) {
            return input;
        }

        std::cout << prompt;
    }
}

std::string get_answer_input() {
    std::string input;

    while (true) {
        std::getline(std::cin, input);

        if (input.size() == 1) {
            char answer = static_cast<char>(std::toupper(input[0]));

            if (answer == 'A' || answer == 'B' || answer == 'C') {
                return std::string(1, answer);
            }
        }

        std::cout << "Invalid answer. Enter A, B, or C: ";
    }
}

void send_input(int socket, const std::string& input) {
    send(socket, input.c_str(), input.size(), 0);
}

void run_game_loop(int socket) {
    char buffer[BUFFER_SIZE] = {};

    while (true) {
        std::memset(buffer, 0, BUFFER_SIZE);

        int bytes_received = recv(socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received <= 0) {
            break;
        }

        std::string message(buffer, bytes_received);
        std::cout << message;

        if (message.find("Enter name") != std::string::npos) {
            send_input(socket, get_required_input("Enter name: "));
        } else if (message.find("Your answer") != std::string::npos) {
            send_input(socket, get_answer_input());
        }
    }

    std::cout << "\nDisconnected from server.\n";
}

int main() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket < 0) {
        std::cerr << "Failed to create client socket.\n";
        return 1;
    }

    sockaddr_in server_address = {};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        std::cerr << "Invalid server address.\n";
        close(client_socket);
        return 1;
    }

    if (connect(client_socket, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) < 0) {
        std::cerr << "Failed to connect to server.\n";
        close(client_socket);
        return 1;
    }

    run_game_loop(client_socket);
    close(client_socket);

    return 0;
}