#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>
#include <string>
#include <optional>
#include <vector>
#include <mutex>

#include "../defines.hpp"
#include "messaging/messaging.hpp"

class ClientSession {
    int _conn_fd = -1;
    struct sockaddr_in _client_info;

    Messenger _messenger;
    std::mutex _mtx;

    std::string nickname;

public:
    explicit ClientSession(int conn_fd, struct sockaddr_in client_info);
    ~ClientSession();
    
    ClientSession(const ClientSession& other) = delete;
    ClientSession& operator=(const ClientSession& other) = delete;
    
    ClientSession(ClientSession&& other);
    ClientSession& operator=(ClientSession&& other);

    void recvHello();
    void sendWelcome(uint16_t port);
    void auth();

    void sendPong();

    void send(const Message& msg, int fd = -1);
    std::optional<Message> recv();

    void close();
    bool isActive() const {return _conn_fd != -1; }

    std::string getClientName() const { return nickname; }
    int fd() const { return _conn_fd; }
};