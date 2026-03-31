#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>

#include <iostream>

#include "../messaging.hpp"

class Client {
    struct sockaddr_in _server_info;
    int _socket_fd = -1;

    std::string _ip_addr;
    uint16_t _port;

    Messenger _messenger;

public:
    Client() = default;
    Client(const std::string& ip_addr, uint16_t port);

    Client(const Client& other) = delete;
    Client& operator=(const Client& other) = delete;
    
    Client(Client&& other) = default;
    Client& operator=(Client&& other) = default;

    void connect();

    void sendHello();
    void processMessaging();

    void close();

    void send(const Message& msg);
    std::optional<Message> recv();

    void recvWelcome();
};