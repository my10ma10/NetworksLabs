#pragma once

#include "defines.hpp"

#include <arpa/inet.h>

#include <stdexcept>
#include <cctype>
#include <iostream>
#include <optional>

class Messenger {
public:
    Messenger() = default;

    void sendMsg(const Message& msg, int conn_fd);
    std::optional<Message> recvMsg(int conn_fd);
};

Message stringToMsg(const std::string& str, MessageType type);
std::string convertToNick_Msg(const std::string& input);
std::pair<std::string, std::string> convertFromNick_Msg(const std::string& input);