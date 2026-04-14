#include <iostream>
#include <cstring>
#include <thread>
#include <atomic>
#include <condition_variable>

#include "../defines.hpp"
#include "client.hpp"

std::atomic<bool> is_reconnecting{false};
std::mutex reconnect_mtx;
std::condition_variable reconnect_cv;

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
                    std::cout << "\r" << client.getFormattedIpPort() << \
                            msg->payload << "\n> " << std::flush;
                    break;
                case MSG_PRIVATE:
                    std::cout << "\r" << "[PRIVATE]: " << \
                            msg->payload << "\n> " << std::flush;
                    break;
                case MSG_PONG:
                    std::cout << "\rPONG\n> " << std::flush;
                    break;
                case MSG_WELCOME:
                    std::cout << "\r" << client.getFormattedIpPort() << \
                            msg->payload << "\n> " << std::flush;
                    break;
                case MSG_ERROR:
                    std::cerr << "Error msg: " << msg->payload << std::endl;
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
                if (input_str == "/ping") {
                    Message msg = {5, MSG_PING, "PING"};
                    client.send(msg);
                }
                else if (input_str == "/quit") {
                    is_running = false;
                    client.shutdown();
                    break;
                }
                else if (input_str.substr(0, 3) == "/w " && input_str.size() > 3) {
                    
                    Message private_msg = stringToMsg(
                        convertToNick_Msg(input_str), MSG_PRIVATE);

                    client.send(private_msg);
                }
                else {
                    std::cerr << "Unexpected command\n";
                }
            }
            else {
                std::unique_lock lock(reconnect_mtx);
                reconnect_cv.wait(lock, [] { return !is_reconnecting.load(); });

                Message text_msg = stringToMsg(input_str, MSG_TEXT);
                
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
            return;
        }
        catch (const std::exception& ex) {
            std::cerr << "Reconnecting in 2 seconds (" << ex.what() << ")\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            client.reset();
        }
    }
}
