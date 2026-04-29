#include <nlohmann/json.hpp>
#include <nlohmann/adl_serializer.hpp>
#include "defines.hpp"

using json = nlohmann::json;


namespace nlohmann {
    template <>
    struct adl_serializer<MessageEx> {
        static void to_json(json& j, const MessageEx& msg) {
        j = json {
            {"length", msg.length},
            {"type", msg.type},
            {"msg_id", msg.msg_id},
            {"sender", std::string(msg.sender)},
            {"receiver", std::string(msg.receiver)},
            {"timestamp", static_cast<int64_t>(msg.timestamp)},
            {"payload", std::string(msg.payload)}
        };
    }

        static void from_json(const json& j, MessageEx& msg) {
            j.at("length").get_to(msg.length);
            j.at("type").get_to(msg.type);
            j.at("msg_id").get_to(msg.msg_id);
            
            std::strncpy(msg.sender, j.at("sender").get<std::string>().c_str(), MAX_NAME - 1);
            msg.sender[MAX_NAME - 1] = '\0';
            
            std::strncpy(msg.receiver, j.at("receiver").get<std::string>().c_str(), MAX_NAME - 1);
            msg.receiver[MAX_NAME - 1] = '\0';
            
            j.at("timestamp").get_to(msg.timestamp);
            
            std::strncpy(msg.payload, j.at("payload").get<std::string>().c_str(), MAX_PAYLOAD - 1);
            msg.payload[MAX_PAYLOAD - 1] = '\0';
        }
    };
}