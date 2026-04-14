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
        throw std::runtime_error("Connection with the server failed");
    }

    std::cout << "Connected\n";
}

void Client::sendHello(const Message& msg) {
    Client::send(msg);
}

void Client::recvWelcome() {
    auto welcome_msg = Client::recv();

    if (!welcome_msg.has_value()) {
        throw std::runtime_error("Nullopt welcome msg");
    }

    std::cout << msgToString(welcome_msg.value()) << std::endl;
}

void Client::auth(Message msg) {
    msg.type = MSG_AUTH;

    Client::send(msg);

    auto auth_msg = Client::recv();

    if (!auth_msg.has_value()) {
        throw std::runtime_error("Nullopt auth msg");
    }
    if (auth_msg->type != MSG_AUTH) {
        throw std::runtime_error("Unexpected msg type: " + std::to_string(auth_msg->type));
    }
    if (auth_msg->type == MSG_ERROR) {
        throw std::runtime_error("Auth error: " +  msgToString(auth_msg.value()));
    }

    Logger::log(5, "Session", "authentication success");
}

Message Client::enterNickname() {
    std::cout << "Enter nickname: " << std::endl;
    std::string nickname_str;
    std::getline(std::cin, nickname_str);

    return stringToMsg(nickname_str, MSG_HELLO);
}

void Client::close() {
    ::close(_socket_fd);
    _socket_fd = -1;
}

void Client::shutdown() {
    ::shutdown(_socket_fd, SHUT_WR);
    _socket_fd = -1;
}

void Client::send(const Message& msg) {
    _messenger.sendMsg(msg, _socket_fd);
}

std::optional<Message> Client::recv() {
    return _messenger.recvMsg(_socket_fd);
}

std::string Client::getFormattedIpPort() const {
    std::string res;
    res.resize(20);
    std::sprintf(res.data(), "[%s:%d] ", _ip_addr.data(), _port);

    return res;
}

void Client::reset() {
    Client::close();
}
