#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr int PORT = 12345;
constexpr int MAX_PLAYERS = 3;
constexpr int BUFFER_SIZE = 1024;
constexpr int ROUND_COUNT = 3;
constexpr int POINTS_PER_CORRECT_ANSWER = 10;

struct Player {
    int socket;
    std::string name;
    int score;
    char answer;
};

struct Question {
    std::string text;
    std::map<char, std::string> choices;
    char correct_answer;
    bool used;
};

std::mutex players_mutex;
std::vector<Player> players;

std::vector<Question> questions = {
    {
        "Sino ang Pambansang Bayani ng Pilipinas?",
        {{'A', "Jose Rizal"}, {'B', "Andres Bonifacio"}, {'C', "Emilio Aguinaldo"}},
        'A',
        false
    },
    {
        "Kailan idineklara ang kalayaan ng Pilipinas?",
        {{'A', "Hulyo 12, 1898"}, {'B', "Hunyo 12, 1898"}, {'C', "Hunyo 12, 1889"}},
        'B',
        false
    },
    {
        "Anong bahagi ng pananalita ang mga salitang ako, ikaw, at sila?",
        {{'A', "Pang-abay"}, {'B', "Pang-uri"}, {'C', "Panghalip"}},
        'C',
        false
    },
    {
        "Ano ang tawag sa pinakamataas na bundok sa Pilipinas?",
        {{'A', "Mt. Banahaw"}, {'B', "Mt. Apo"}, {'C', "Mt. Pulag"}},
        'B',
        false
    },
    {
        "Sino ang kasalukuyang pambansang alagad ng sining sa Panitikan?",
        {{'A', "Virgilio Almario"}, {'B', "Nick Joaquin"}, {'C', "F. Sionil Jose"}},
        'A',
        false
    },
    {
        "Alin sa mga ito ang isang epikong-bayan ng mga Ifugao?",
        {{'A', "Biag ni Lam-ang"}, {'B', "Hudhud"}, {'C', "Ibalon"}},
        'B',
        false
    },
    {
        "Saan matatagpuan ang Banaue Rice Terraces?",
        {{'A', "Ifugao"}, {'B', "Benguet"}, {'C', "Ilocos"}},
        'A',
        false
    }
};

void send_message(int socket, const std::string& message) {
    send(socket, message.c_str(), message.size(), 0);
}

std::string receive_message(int socket) {
    char buffer[BUFFER_SIZE] = {};
    int bytes_received = recv(socket, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_received <= 0) {
        return "";
    }

    return std::string(buffer, bytes_received);
}

std::string trim(const std::string& value) {
    size_t start = value.find_first_not_of(" \n\r\t");
    size_t end = value.find_last_not_of(" \n\r\t");

    if (start == std::string::npos) {
        return "";
    }

    return value.substr(start, end - start + 1);
}

char normalize_answer(const std::string& answer) {
    std::string trimmed_answer = trim(answer);

    if (trimmed_answer.empty()) {
        return '\0';
    }

    return static_cast<char>(std::toupper(trimmed_answer[0]));
}

Question* get_next_question() {
    for (Question& question : questions) {
        if (!question.used) {
            question.used = true;
            return &question;
        }
    }

    return nullptr;
}

std::string format_question(const Question& question) {
    std::string output = question.text + "\n";

    for (const auto& choice : question.choices) {
        output += std::string(1, choice.first) + ". " + choice.second + "\n";
    }

    return output;
}

void handle_player_join(int client_socket) {
    send_message(client_socket, "Enter name: ");
    std::string name = trim(receive_message(client_socket));

    if (name.empty()) {
        name = "Player " + std::to_string(players.size() + 1);
    }

    {
        std::lock_guard<std::mutex> lock(players_mutex);
        players.push_back({client_socket, name, 0, '\0'});
    }

    std::cout << name << " joined the game.\n";
}

void ask_question_to_all_players(const Question& question, int round_number) {
    std::string question_text =
        "\nRound " + std::to_string(round_number) + "\n" +
        format_question(question) +
        "\nYour answer (A/B/C): ";

    for (Player& player : players) {
        send_message(player.socket, question_text);
        player.answer = normalize_answer(receive_message(player.socket));
    }
}

void update_scores(const Question& question, std::vector<Player*>& current_players) {
    for (Player* player : current_players) {
        if (player->answer == question.correct_answer) {
            player->score += POINTS_PER_CORRECT_ANSWER;
        }
    }
}

std::string build_scoreboard(const std::string& title, const std::vector<Player*>& current_players) {
    std::string scoreboard = "\n" + title + "\n------------------\n";

    for (const Player* player : current_players) {
        scoreboard += player->name + ": " + std::to_string(player->score) + " points\n";
    }

    return scoreboard;
}

void broadcast(const std::string& message) {
    for (const Player& player : players) {
        send_message(player.socket, message);
    }
}

std::vector<Player*> get_highest_scoring_players(const std::vector<Player*>& current_players) {
    int highest_score = 0;

    for (const Player* player : current_players) {
        highest_score = std::max(highest_score, player->score);
    }

    std::vector<Player*> highest_scoring_players;

    for (Player* player : current_players) {
        if (player->score == highest_score) {
            highest_scoring_players.push_back(player);
        }
    }

    return highest_scoring_players;
}

void run_tiebreaker(std::vector<Player*> winners) {
    broadcast("\nEntering tiebreaker round...\n");

    while (winners.size() > 1) {
        Question* question = get_next_question();

        if (question == nullptr) {
            broadcast("\nNo more questions available for the tiebreaker.\n");
            return;
        }

        std::string question_text =
            "\nTIEBREAKER QUESTION\n" +
            format_question(*question) +
            "\nYour answer (A/B/C): ";

        for (Player* player : winners) {
            send_message(player->socket, question_text);
            player->answer = normalize_answer(receive_message(player->socket));
        }

        update_scores(*question, winners);
        broadcast(build_scoreboard("TIEBREAKER SCOREBOARD", winners));

        winners = get_highest_scoring_players(winners);
    }

    broadcast("\nTiebreaker Winner: " + winners[0]->name + "\n");
}

void run_game() {
    for (int round = 1; round <= ROUND_COUNT; ++round) {
        Question* question = get_next_question();

        if (question == nullptr) {
            break;
        }

        ask_question_to_all_players(*question, round);

        std::vector<Player*> active_players;
        for (Player& player : players) {
            active_players.push_back(&player);
        }

        update_scores(*question, active_players);
        broadcast(build_scoreboard("SCOREBOARD", active_players));
    }

    std::vector<Player*> final_players;
    for (Player& player : players) {
        final_players.push_back(&player);
    }

    std::vector<Player*> winners = get_highest_scoring_players(final_players);
    std::string final_results = build_scoreboard("FINAL RESULTS", final_players);

    if (winners.size() == 1) {
        final_results += "\nWinner: " + winners[0]->name + "\n";
        broadcast(final_results);
        return;
    }

    final_results += "\nIt's a tie between: ";

    for (const Player* winner : winners) {
        final_results += winner->name + " ";
    }

    final_results += "\n";
    broadcast(final_results);

    run_tiebreaker(winners);
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0) {
        std::cerr << "Failed to create server socket.\n";
        return 1;
    }

    int option = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    sockaddr_in server_address = {};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_socket, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) < 0) {
        std::cerr << "Failed to bind server socket.\n";
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, MAX_PLAYERS) < 0) {
        std::cerr << "Failed to listen for players.\n";
        close(server_socket);
        return 1;
    }

    std::cout << "Waiting for " << MAX_PLAYERS << " players to connect...\n";

    std::vector<std::thread> player_threads;

    for (int i = 0; i < MAX_PLAYERS; ++i) {
        sockaddr_in client_address = {};
        socklen_t client_address_size = sizeof(client_address);

        int client_socket = accept(
            server_socket,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_address_size
        );

        if (client_socket < 0) {
            std::cerr << "Failed to accept player connection.\n";
            continue;
        }

        player_threads.emplace_back(handle_player_join, client_socket);
    }

    for (std::thread& player_thread : player_threads) {
        player_thread.join();
    }

    if (players.empty()) {
        std::cerr << "No players connected. Closing server.\n";
        close(server_socket);
        return 1;
    }

    std::cout << "All players joined. Starting game...\n";
    run_game();

    for (const Player& player : players) {
        close(player.socket);
    }

    close(server_socket);
    return 0;
}