#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>
#include <cstring>

#include <iostream>

#include "../defines.hpp"
#include "../messaging.hpp"

class Server {
    struct sockaddr_in _server_info, _client_info;
    int _socket_fd = -1;
    int _conn_fd = -1;

    std::string _ip_addr;
    uint16_t _port;

    Messenger _messenger;

public:
    Server() = default;
    Server(const std::string& ip_addr, uint16_t port);

    ~Server();

    void close();

    void recvHello();
    void sendWelcome();

    void sendPong();

    Server(const Server& other) = default;
    Server& operator=(const Server& other) = default;
    
    Server(Server&& other) = delete;
    Server& operator=(Server&& other) = delete;

    void bind();
    void listen(int attempts);

    void accept();
    
    void send(const Message& msg);
    std::optional<Message> recv();

    std::string getIpPort() const;

};