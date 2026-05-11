#pragma once

#include "defines.hpp"

#include <arpa/inet.h>

#include <stdexcept>
#include <cctype>
#include <iostream>
#include <optional>

#include "logger/logger.hpp"
#include "json/json.hpp"

using json = nlohmann::json;

class Messenger {
public:
    Messenger() = default;

    void sendMsg(const MessageEx& msg, int conn_fd);
    std::optional<MessageEx> recvMsg(int conn_fd);
};

MessageEx stringToMsg(const std::string& str, MessageType type);
std::string msgToString(const MessageEx& msg);

std::pair<std::string, std::string> convertToNick_Msg(const std::string& input);

void printTextMessage(const MessageEx& msg);
void printPrivateMessage(const MessageEx& msg);
void printOfflineMessage(const MessageEx& msg);

std::string jsonToString(const json& j);