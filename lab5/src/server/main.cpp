#include "server.hpp"

#include "thread_pool/thread_pool.hpp"
#include "session_registry.hpp"

int main() {
    Server server("127.0.0.1", 8080);
    server.bind();
    server.listen(10);

    SessionRegistry registry;
    ThreadPool pool;
    
    while (true) {
        ClientSession session = server.accept();


        pool.enqueueConnection([session = std::move(session), &server, &registry]() mutable {
            registry.add(session);

            try {
                session.recvHello();
                session.sendWelcome(server.getPort());

                session.auth();
                registry.registerNickname(session.fd(), session.getClientName());

                registry.deliverOffline(session.getClientName(), &session);

                while (session.isActive()) {
                    auto msg = session.recv();
                    if (!msg.has_value()) {
                        std::cerr << "User [" << session.getClientName() \
                                    <<  "] disconnected" << std::endl;
                        break;
                    }

                    switch (msg->type) {
                        case MSG_TEXT: {
                            Logger::log("Application", "handle MSG_TEXT");

                            std::cout << session.getClientName() << " " \
                                << server.getFormattedIpPort() << msgToString(msg.value()) << std::endl;
                            
                            registry.broadcast(msg.value(), session.fd(), session.getClientName());                            
                            break;
                        }
                        case MSG_PRIVATE: {
                            Logger::log("Application", "handle MSG_PRIVATE");

                            std::string target_nickname = msg->receiver;
                            std::string message_str = msgToString(msg.value());

                            MessageEx private_msg = stringToMsg(message_str, MSG_PRIVATE);
                            std::memcpy(private_msg.receiver, target_nickname.data(), 
                                        std::min(target_nickname.size(), (size_t)MAX_NAME - 1));

                            registry.sendPrivate(private_msg, target_nickname, session.getClientName());
                            break;
                        }
                        case MSG_PING: {
                            Logger::log("Application", "handle MSG_PING");

                            session.sendPong();
                            break;
                        }
                        case MSG_BYE: {
                            Logger::log("Application", "handle MSG_BYE");

                            session.close();
                            registry.remove(session.fd());
                            return;
                        }
                        case MSG_LIST: {
                            Logger::log("Application", "handle MSG_LIST");

                            std::string user_list = "Online users\n" + registry.getUserList();
                            MessageEx reply = stringToMsg(user_list, MSG_SERVER_INFO);
                            session.send(reply);
                            break;
                        }
                        case MSG_HISTORY: {
                            Logger::log("Application", "handle MSG_HISTORY");

                            int n = 0;
                            std::string payload = msgToString(msg.value());
                            if (!payload.empty()) {
                                try { n = std::stoi(payload); } catch (...) { n = 0; }
                            }

                            std::string history = registry.getHistory(n); // читает из JSON-файла
                            MessageEx reply = stringToMsg(history, MSG_HISTORY_DATA);
                            session.send(reply);
                            break;
                        }
                        case MSG_ERROR: {
                            Logger::log("Application", "handle MSG_ERROR");

                            std::cerr << "Error: " << msgToString(msg.value()) << std::endl;
                            break;
                        }
                        default: {
                            std::cerr << "Unexpected msg type: " << static_cast<int>(msg->type) << std::endl;
                            return;
                        }
                    }
                }
            }
            catch (const std::runtime_error& ex) {
                std::cout << "Session runtime error: " << ex.what() << std::endl;
            }
            catch (const std::exception& ex) {
                std::cout << "Session error: " << ex.what() << std::endl;
            }
            catch (...) {
                std::cout << "Unexpected error\n";
            }

            registry.remove(session.fd());
        });
    }
    return 0;
}