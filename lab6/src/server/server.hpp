#pragma once

#include <unistd.h>
#include <cstring>
#include <iostream>

#include "client_session.hpp"

class Server {
    struct sockaddr_in _server_info;
    int _socket_fd = -1;

    std::string _ip_addr;
    uint16_t _port;

public:
    Server() = default;
    Server(const std::string& ip_addr, uint16_t port);

    ~Server();

    void close();

    Server(const Server& other) = default;
    Server& operator=(const Server& other) = default;
    
    Server(Server&& other) = delete;
    Server& operator=(Server&& other) = delete;

    void bind();
    void listen(int attempts);

    ClientSession accept();
    

    std::string getFormattedIpPort() const;
    bool isActive() const { return _socket_fd != -1; }

    uint16_t getPort() const { return _port; }
};