#pragma once
#include <mutex>
#include <unordered_map>

#include "client_session.hpp"

class SessionRegistry {
    std::mutex _mtx;
    std::unordered_map<int, ClientSession*> _sessions;

public:
    SessionRegistry() = default;

    void add(ClientSession& session);
    void remove(int fd);
    void broadcast(const Message& msg, int sender_fd, const std::string& sender_name);

private:
    Message formatBroadcastMsg(const Message& msg, const std::string& sender_name);
};