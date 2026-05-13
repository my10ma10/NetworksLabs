#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>
#include <string>
#include <optional>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "../defines.hpp"
#include "messaging/messaging.hpp"
#include "thread_pool/thread_safe_queue.hpp"

class ClientSession {
    int _conn_fd = -1;
    struct sockaddr_in _client_info;

    Messenger _messenger;
    std::mutex _mtx;
    
    std::thread _ack_reader;
    ThreadSafeQueue<MessageEx> _inbox;

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

    void startAckReader();
    void sendAckFor(uint32_t msg_id);

    void sendPong(const MessageEx& pong);

    void send(const MessageEx& msg, int fd = -1);
    std::optional<MessageEx> recv();

    void close();
    bool isActive() const {return _conn_fd != -1; }

    std::string getClientName() const { return nickname; }
    int fd() const { return _conn_fd; }

    void rawSend(const MessageEx& msg, int fd = -1);
    std::optional<MessageEx> rawRecv();
};