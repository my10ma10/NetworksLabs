#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <mutex>
#include <thread>

#include "thread_pool/thread_safe_queue.hpp"
#include "messaging/messaging.hpp"

struct PingStats;

class Client {
    struct sockaddr_in _server_info;
    int _socket_fd = -1;

    std::string _ip_addr;
    uint16_t _port;

    Messenger _messenger;
    ThreadSafeQueue<MessageEx> _inbox;
    std::thread _ack_reader;

    std::string _nickname;
    
    std::mutex _mtx, _ping_mtx;
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> _ping_sent;
    std::unordered_map<uint32_t, double> _ping_rtt;

public:
    Client() = default;
    Client(const std::string& ip_addr, uint16_t port);

    Client(const Client& other) = delete;
    Client& operator=(const Client& other) = delete;
    
    Client(Client&& other) = default;
    Client& operator=(Client&& other) = default;

    void connect();

    void sendHello(const MessageEx& msg);
    void recvWelcome();

    void startAckReader();
    void notifyAck(uint32_t expected_msg_id);

    void registerPing(uint32_t msg_id);
    void registerPong(uint32_t msg_id);

    void saveNetDiag(double rtt_avg, double jitter_avg, double loss,
                          const std::vector<PingStats>& stats);
    
    void auth(MessageEx msg);
    MessageEx enterNickname();

    void close();
    void shutdown();
    void reset();

    void send(const MessageEx& msg);
    std::optional<MessageEx> recv();

    void printPingResults(std::vector<PingStats>& stats);

    std::optional<double> getRtt(uint32_t msg_id);
    std::string getFormattedIpPort() const;
    std::string getNickname() const;

    void rawSend(const MessageEx& msg);
    std::optional<MessageEx> rawRecv();
};

struct PingStats {
    uint32_t msg_id;
    std::chrono::steady_clock::time_point sent_at;
    std::optional<double> rtt_ms;
};