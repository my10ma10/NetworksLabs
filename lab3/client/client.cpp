#include "client.hpp"

#include <cstring>

Client::Client(const std::string &ip_addr, uint16_t port) :
    _ip_addr(ip_addr), _port(port)
{
}

void Client::connect() {
    _socket_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if (_socket_fd == -1) {
        throw std::runtime_error("Client socket creation failed");
    }

    _server_info.sin_family = AF_INET; 
    _server_info.sin_addr.s_addr = inet_addr(_ip_addr.c_str());
    _server_info.sin_port = htons(_port); 

    if (::connect(_socket_fd, (struct sockaddr *)&_server_info, sizeof(_server_info)) == -1) {
        throw std::runtime_error("Connection with the server failed...\n");
    }

    std::cout << "Connected\n";
}

void Client::sendHello() {
    Message msg;

    std::cout << "Enter nickname: " << std::endl;
    std::string hello_str;
    std::getline(std::cin, hello_str);

    msg.type = MSG_HELLO;
    msg.length = hello_str.size();
    
    std::memset(msg.payload, 0, MAX_PAYLOAD);
    std::memcpy(msg.payload, hello_str.data(), hello_str.size());

    Client::send(msg);
}

void Client::close() {
    ::close(_socket_fd);

    _socket_fd = -1;
    
    std::cout << "Disconnected\n";
}

void Client::send(const Message &msg) {
    _messenger.sendMsg(msg, _socket_fd);
}

std::optional<Message> Client::recv() {
    return _messenger.recvMsg(_socket_fd);
}

void Client::recvWelcome() {
    auto welcome_msg = Client::recv();

    if (!welcome_msg.has_value()) {
        throw std::runtime_error("Nullopt welcome msg");
    }

    std::cout << welcome_msg->payload << std::endl;
}
