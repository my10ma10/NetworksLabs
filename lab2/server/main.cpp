#include "server.hpp"
#include <type_traits>

int main() {
    try {
        Server server("127.0.0.1", 8080);

        server.bind();
        server.listen(1);
        server.accept();

        server.recvHello();

        server.sendWelcome();

        while (true) {
            auto recv_res = server.recv();
            if (!recv_res.has_value()) {
                std::cout << "Client disconnected\n";
                break;                
            }

            Message recv_msg = recv_res.value();

            switch (recv_msg.type)
            {
            case MSG_TEXT: {
                std::cout << server.getIpPort() << recv_msg.payload << std::endl;
                break;
            }
            case MSG_PING: {
                server.sendPong();
                break;
            }
            case MSG_BYE: {
                server.close();
                return 0;
            }
            
            default: {
                std::cerr << "Unexpected msg type: " << static_cast<int>(recv_msg.type) << std::endl;
                return 0;
            }
            }
        }
    }
    catch (const std::runtime_error& ex) {
        std::cout << "Server runtime error: " << ex.what() << std::endl;
    }
    catch (const std::exception& ex) {
        std::cout << "Server undefined exception: " << ex.what() << std::endl;
    }
}