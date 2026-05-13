#include <iostream>
#include <cstring>
#include <thread>
#include <atomic>
#include <condition_variable>

#include "../defines.hpp"
#include "client.hpp"

#include "../time_formatting.cpp"

std::atomic<bool> is_reconnecting{false};
std::mutex reconnect_mtx;
std::condition_variable reconnect_cv;
std::vector<PingStats> stats;

void connectWithRetry(Client& client);

int main() {
    Client client("127.0.0.1", 8080);

    std::atomic<bool> is_running{true};
    
    connectWithRetry(client);

    std::thread msg_reader([&client, &is_running]() {
        while (is_running) {
            auto msg = client.recv();
            if (!msg.has_value()) {
                if (!is_running) break;

                if (!is_reconnecting.exchange(true)) {
                    std::cerr << "Connection lost, reconnecting\n";

                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    connectWithRetry(client);
                    
                    is_reconnecting = false;
                    reconnect_cv.notify_all();
                }
                continue;
            }

            switch (msg->type) {
                case MSG_TEXT: 
                {
                    if (std::string(msg->payload).substr(0, 9) == "[OFFLINE]") {
                        printOfflineMessage(msg.value());
                    } 
                    else {
                        printTextMessage(msg.value());
                    }
                    break;
                }
                case MSG_PRIVATE:
                {
                    printPrivateMessage(msg.value());
                    break;
                }
                case MSG_PONG:
                {
                    client.registerPong(msg->msg_id);
                    std::cout << "\rPONG\n> " << std::flush;
                    break;
                }
                case MSG_WELCOME:
                    std::cout << "\r" << client.getFormattedIpPort() << \
                            msgToString(msg.value()) << "\n> " << std::flush;
                    break;
                case MSG_ERROR:
                    std::cerr << "Error msg: " << msgToString(msg.value()) << std::endl;
                    break;
                case MSG_SERVER_INFO:
                    std::cout << "\r[SERVER]: " << msgToString(msg.value()) << "\n> " << std::flush;
                    break;

                case MSG_HISTORY_DATA:
                    std::cout << "\r" << "[HISTORY:]\n\r" << msgToString(msg.value()) << "\n> " << std::flush;
                    break;
                default:
                    std::cerr << "\rUnexpected msg type: " 
                            << static_cast<int>(msg->type) << "\n> " << std::flush;
                    break;
            }
        }
    });

    try {
        while (true) {
            std::cout << "> ";

            std::string input_str;
            std::getline(std::cin, input_str);

            if (input_str.front() == '/') {
                if (input_str == "/help") {
                    std::cout << "Available commands:\n"
                            << "/help\n/list\n/history\n/history N\n"
                            << "/quit\n/w <nick> <message>\n/ping\n"
                            << "Tip: packets never sleep\n";
                }
                else if (input_str == "/ping" || input_str.substr(0, 6) == "/ping ") {
                    int n = 10; 
                    if (input_str.size() > 6) {
                        try {
                            n = std::stoi(input_str.substr(6));
                            if (n <= 0) throw std::invalid_argument("");
                        } 
                        catch (...) {
                            std::cerr << "Usage: /ping N (N > 0)\n";
                            continue;
                        }
                    }


                    for (int i = 0; i < n; ++i) {
                        MessageEx ping = stringToMsg("PING", MSG_PING);
                        PingStats s;
                        s.msg_id   = ping.msg_id;
                        s.sent_at  = std::chrono::steady_clock::now();
                        stats.push_back(s);

                        client.registerPing(ping.msg_id);
                        client.send(ping);
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }

                    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);

                    while (std::chrono::steady_clock::now() < deadline) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                }
                else if (input_str == "/netdiag") {
                    if (!stats.empty()) {
                        client.printPingResults(stats);
                    }
                    else {
                        std::cout << "First you need to make a diagnosis\n";
                    }

                }
                else if (input_str == "/quit") {
                    is_running = false;
                    client.shutdown();
                }
                else if (input_str == "/list") {
                    MessageEx msg = stringToMsg("", MSG_LIST);
                    client.send(msg);
                }

                else if (input_str == "/history") {
                    MessageEx msg = stringToMsg("", MSG_HISTORY);
                    client.send(msg);
                }
                else if (input_str.substr(0, 9) == "/history " && input_str.size() > 9) {
                    std::string n_str = input_str.substr(9);
                    try {
                        int n = std::stoi(n_str);
                        if (n <= 0) throw std::invalid_argument("n <= 0");
                        MessageEx msg = stringToMsg(std::to_string(n), MSG_HISTORY);
                        client.send(msg);
                    }
                    catch (const std::exception&) {
                        std::cerr << "Invalid argument: /history N requires positive integer\n";
                    }
                }
                else if (input_str.substr(0, 3) == "/w " && input_str.size() > 3) {
                    auto [nickname, message_str] = convertToNick_Msg(input_str);
                    
                    MessageEx private_msg = stringToMsg(message_str, MSG_PRIVATE);
                    std::memcpy(private_msg.receiver, nickname.data(), 
                                std::min(nickname.size(), (size_t)MAX_NAME - 1));
                    std::memcpy(private_msg.sender, client.getNickname().data(), 
                                std::min(client.getNickname().size(), (size_t)MAX_NAME - 1));

                    client.send(private_msg);
                }
                else {
                    std::cerr << "Unexpected command\n";
                }
            }
            else {
                std::unique_lock lock(reconnect_mtx);
                reconnect_cv.wait(lock, [] { return !is_reconnecting.load(); });

                MessageEx text_msg = stringToMsg(input_str, MSG_TEXT);
                
                std::memcpy(text_msg.sender, client.getNickname().data(), 
                            std::min(client.getNickname().size(), (size_t)MAX_NAME - 1));
                
                client.send(text_msg);
            }
        }
    }
    catch (const std::runtime_error& ex) {
        std::cout << "Client runtime error: " << ex.what() << std::endl;
    }
    catch (const std::exception&) {
        std::cerr << "Send failed, waiting for reconnect\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }


    if (msg_reader.joinable()) {
        msg_reader.join();
    }
    return 0;
}

void connectWithRetry(Client& client) {
    while (true) {
        try {
            client.connect();

            auto nickname_msg = client.enterNickname();

            client.sendHello(nickname_msg);
            client.recvWelcome();

            client.auth(nickname_msg);

            client.startAckReader();
            return;
        }
        catch (const std::exception& ex) {
            std::cerr << "Reconnecting in 2 seconds (" << ex.what() << ")\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            client.reset();
        }
    }
}
