#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>
#include <string>
#include <optional>
#include <unordered_map>

#include "../defines.hpp"
#include "../messaging.hpp"

class ClientSession {
    int _conn_fd = -1;
    struct sockaddr_in _client_info;

    Messenger _messenger;

    std::unordered_map<int, std::string> _connfd_nicknames_map;

public:
    explicit ClientSession(int conn_fd, struct sockaddr_in client_info);
    ~ClientSession();
    
    ClientSession(const ClientSession& other) = delete;
    ClientSession& operator=(const ClientSession& other) = delete;
    
    ClientSession(ClientSession&& other);
    ClientSession& operator=(ClientSession&& other);

    void recvHello();
    void sendWelcome(uint16_t port);
    void sendPong();

    void send(const Message& msg);
    std::optional<Message> recv();

    void close();
    bool isActive() const {return _conn_fd != -1; }
    std::string getName() const { return _connfd_nicknames_map.at(_conn_fd); }
};