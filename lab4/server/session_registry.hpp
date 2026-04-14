#pragma once
#include <mutex>
#include <unordered_map>

#include "client_session.hpp"

class SessionRegistry {
    std::mutex _mtx;
    std::unordered_map<int, ClientSession*> _sessions;

    std::unordered_map<std::string, int> _nickname_to_fd;

public:
    SessionRegistry() = default;

    void add(ClientSession& session);
    void remove(int fd);

    void broadcast(const Message& msg, int sender_fd, const std::string& sender_name);
    void sendPrivate(const Message& msg, const std::string& target_nickname,
                 const std::string& sender_name);

void registerNickname(int fd, const std::string& nickname);
private:
    Message formatBroadcastMsg(const Message& msg, const std::string& sender_name);
};