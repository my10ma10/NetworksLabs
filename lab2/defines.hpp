#pragma once

#include <cstdint>

inline constexpr int MAX_PAYLOAD = 1024;

struct Message {
    uint32_t length;
    uint8_t type;
    char payload[MAX_PAYLOAD];
};

enum MessageType : uint8_t {
    MSG_HELLO = 1,
    MSG_WELCOME,
    MSG_TEXT,
    MSG_PING,
    MSG_PONG,
    MSG_BYE,
};