#include <iostream>
#include <cstring>

#include "../defines.hpp"
#include "client.hpp"

int main() {
    try {
        Client client("127.0.0.1", 8080);
        client.connect();

        client.sendHello();

        client.recvWelcome();

        while (true) {
            std::cout << "> ";

            std::string input_str;
            std::getline(std::cin, input_str);

            if (input_str.front() == '/') {
                if (input_str == "/ping") {
                    Message msg = {5, MSG_PING, "PING"};
                    client.send(msg);

                    auto answer = client.recv();
                    if (!answer.has_value()) {
                        throw std::runtime_error("Nullopt ping-pong answer");
                    }

                    std::cout << answer.value().payload << std::endl;
                }
                else if (input_str == "/quit") {
                    client.close();
                    return 0;
                }
                else {
                    std::cerr << "Unexpected command\n";
                }
            }
            else {
                Message msg;
                std::memset(msg.payload, 0, MAX_PAYLOAD);

                msg.length = input_str.size();
                msg.type = MSG_TEXT;
                std::memcpy(msg.payload, input_str.data(), input_str.size());
                client.send(msg);
            }
        }
    }
    catch (const std::runtime_error& ex) {
        std::cout << "Client runtime error: " << ex.what() << std::endl;
    }
    catch (const std::exception& ex) {
        std::cout << "Client undefined exception: " << ex.what() << std::endl;
    }
    
    return 0;
}