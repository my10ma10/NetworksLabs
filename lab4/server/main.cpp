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

                while (session.isActive()) {
                    auto msg = session.recv();
                    if (!msg.has_value()) {
                        std::cerr << "Client disconnected: " << \
                                session.getClientName() << " " << server.getFormattedIpPort() << std::endl;
                        break;
                    }

                    switch (msg->type) {
                        case MSG_TEXT: {
                            std::cout << session.getClientName() << " " \
                                << server.getFormattedIpPort() << msg->payload << std::endl;
                            
                            registry.broadcast(msg.value(), session.fd(), session.getClientName());                            
                            break;
                        }
                        case MSG_PING: {
                            session.sendPong();
                            break;
                        }
                        case MSG_BYE: {
                            session.close();
                            registry.remove(session.fd());
                            return;
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