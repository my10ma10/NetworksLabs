#pragma once

#include <cstddef>
#include <cstdint>

inline constexpr int MAX_PAYLOAD = 1024;
inline constexpr size_t THREAD_COUNT = 10;

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
    MSG_AUTH,
    MSG_PRIVATE,
    MSG_ERROR,
    MSG_SERVER_INFO
};