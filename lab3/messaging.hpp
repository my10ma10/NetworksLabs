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
