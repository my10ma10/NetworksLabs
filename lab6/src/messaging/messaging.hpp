#pragma once

#include "defines.hpp"

#include <arpa/inet.h>

#include <stdexcept>
#include <cctype>
#include <iostream>
#include <optional>
#include <mutex>
#include <condition_variable>

#include "logger/logger.hpp"
#include "json/json.hpp"

using json = nlohmann::json;

class Messenger {
    std::mutex _ack_mtx;
    std::condition_variable _ack_cv;
    std::unordered_map<uint32_t, bool> _pending_acks;

public:
    Messenger() = default;

    void sendMsg(const MessageEx& msg, int conn_fd);
    std::optional<MessageEx> recvMsg(int conn_fd);

    void notifyACK(uint32_t msg_id);

    size_t rawSend(int fd, std::string_view str);
    ssize_t rawRecv(int fd, std::string& str);

private:
    void sendACK(uint32_t msg_id, int fd);
    bool waitACK(uint32_t expected_msg_id);

    struct timeval initTimeVal();
};

MessageEx stringToMsg(const std::string& str, MessageType type);
std::string msgToString(const MessageEx& msg);

std::pair<std::string, std::string> convertToNick_Msg(const std::string& input);

void printTextMessage(const MessageEx& msg);
void printPrivateMessage(const MessageEx& msg);
void printOfflineMessage(const MessageEx& msg);

std::string jsonToString(const json& j);