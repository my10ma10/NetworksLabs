#pragma once

#include <cstddef>
#include <cstdint>
#include <ctime>

inline constexpr size_t MAX_NAME = 32;
inline constexpr int MAX_PAYLOAD = 256;
inline constexpr size_t MAX_TIME_STR = 32;
inline constexpr size_t THREAD_COUNT = 10;

struct MessageEx {
    uint32_t length;                 // длина полезной части
    uint8_t  type;                   // тип сообщения
    uint32_t msg_id;                 // уникальный идентификатор сообщения
    char     sender[MAX_NAME];       // ник отправителя
    char     receiver[MAX_NAME];     // ник получателя или "" если используется broadcast
    time_t   timestamp;              // время создания
    char     payload[MAX_PAYLOAD];   // текст / данные команды
};


enum MessageType : uint8_t {
    MSG_HELLO        = 1,
    MSG_WELCOME      = 2,
    MSG_TEXT         = 3,
    MSG_PING         = 4,
    MSG_PONG         = 5,
    MSG_BYE          = 6,

    MSG_AUTH         = 7,
    MSG_PRIVATE      = 8,
    MSG_ERROR        = 9,
    MSG_SERVER_INFO  = 10,
	
	MSG_LIST         = 11,  // список пользователей
    MSG_HISTORY      = 12,  // запрос истории
    MSG_HISTORY_DATA = 13,  // ответ с историей
    MSG_HELP         = 14   // справочная информация
};

struct OfflineMsg {  
    char sender[32];  
    char receiver[32];  
    char text[256];  
    time_t timestamp;  
    uint32_t msg_id;  
};