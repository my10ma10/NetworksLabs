#pragma once
#include <mutex>
#include <unordered_map>
#include <queue>

#include "client_session.hpp"

class SessionRegistry {
    std::mutex _mtx;
    std::unordered_map<std::string, std::queue<OfflineMsg>> _offline_queue;
    std::unordered_map<int, ClientSession*> _sessions;

    std::unordered_map<std::string, int> _nickname_to_fd;

    std::string _history_path = "chat_history.jsonl";

public:
    SessionRegistry() = default;

    void appendHistory(const MessageEx& msg, bool delivered, bool is_offline);
    void markDelivered(uint32_t msg_id);

    void add(ClientSession& session);
    void remove(int fd);

    void broadcast(const MessageEx& msg, int sender_fd, const std::string& sender_name);
    void sendPrivate(const MessageEx& msg, const std::string& target_nickname,
                 const std::string& sender_name);
    void deliverOffline(const std::string& nickname, ClientSession* session);

    void registerNickname(int fd, const std::string& nickname);

    std::string getUserList();
    std::string getHistory(int n);

};