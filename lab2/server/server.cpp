#include "server.hpp"

Server::Server(const std::string &ip_addr, uint16_t port) :
    _ip_addr(ip_addr), _port(port)
{
}

Server::~Server() {
    if (_conn_fd != -1 || _socket_fd != -1) {
        close();
    }
}

void Server::close() {
    ::close(_conn_fd);
    ::close(_socket_fd);

    _conn_fd = -1;
    _socket_fd = -1;
}

void Server::recvHello() {
    auto nickname_msg = Server::recv();
    
    if (!nickname_msg.has_value()) {
        throw std::runtime_error("Nullopt hello msg");
    }
    
    std::cout << "Hello " << nickname_msg->payload << std::endl;
}

void Server::sendWelcome()
{
    std::string ip_str = inet_ntoa(_client_info.sin_addr);
    std::string port_str = std::to_string(_port);
    std::string welcome_str = "Welcome " + ip_str + ":" + port_str;

    Message msg;
    msg.length = welcome_str.size();
    msg.type = MSG_WELCOME;

    std::memset(msg.payload, 0, MAX_PAYLOAD);
    std::memcpy(msg.payload, welcome_str.data(), welcome_str.size());
    
    Server::send(msg);
}

void Server::sendPong() {
    Message msg {5, MSG_PONG, "PONG"};
    Server::send(msg);
}

void Server::bind() {
    _socket_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if (_socket_fd == -1) {
        throw std::runtime_error("Server socket creation failed");
    }

    int opt = 1;
    if (setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        throw std::runtime_error("setsockopt failed");
    }

    std::memset(&_server_info, 0, sizeof(_server_info));

    _server_info.sin_family = AF_INET; 
    _server_info.sin_addr.s_addr = INADDR_ANY;
    _server_info.sin_port = htons(_port); 
    
    int status = ::bind(_socket_fd, (struct sockaddr*)(&_server_info), sizeof(_server_info));
    if (status == -1) {
        throw std::runtime_error("Server bind failed");
    }
}

void Server::listen(int attempts) {
    if (::listen(_socket_fd, attempts) == -1) {
        throw std::runtime_error("Server listen failed");
    }
}

void Server::accept() {
    socklen_t cli_len = sizeof(_client_info);
    
    _conn_fd = ::accept(_socket_fd, (struct sockaddr*)(&_client_info), &cli_len);
    if (_conn_fd == -1) {
        throw std::runtime_error("Server accept failed");
    }

    std::cout << "Client connected\n";
}

void Server::send(const Message &msg) {
    _messenger.sendMsg(msg, _conn_fd);
}

std::optional<Message> Server::recv() {
    return _messenger.recvMsg(_conn_fd);
}

std::string Server::getIpPort() const {
    std::string res;
    res.resize(20);
    std::sprintf(res.data(), "[%s:%d]: ", _ip_addr.data(), _port);

    return res.data();
}
