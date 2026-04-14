#include "server.hpp"

Server::Server(const std::string &ip_addr, uint16_t port) :
    _ip_addr(ip_addr), _port(port)
{
}

Server::~Server() {
    Server::close();
}

void Server::close() {
    if (isActive()) {
        ::close(_socket_fd);
        _socket_fd = -1;
    }
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

ClientSession Server::accept() {
    struct sockaddr_in client_info;
    socklen_t info_len = sizeof(client_info);
    
    int conn_fd = ::accept(_socket_fd, (struct sockaddr*)(&client_info), &info_len);
    if (conn_fd == -1) {
        throw std::runtime_error("Server accept failed");
    }

    return ClientSession(conn_fd, client_info);
}


std::string Server::getFormattedIpPort() const {
    std::string res;
    res.resize(20);
    std::sprintf(res.data(), "[%s:%d] ", _ip_addr.data(), _port);

    return res;
}
