#include "messaging.hpp"
#include "../time_formatting.cpp"

#include <cstring>
#include <sstream>
#include <errno.h>
#include <iostream>

void Messenger::sendMsg(const MessageEx& msg, int fd){
    json j = msg;
    std::string j_str = j.dump() + "\n";
    
    const char* buf = j_str.data();
    size_t total_sent = 0;

    while (total_sent < j_str.size()) {
        int sent = ::send(fd, buf + total_sent, j_str.size() - total_sent, 0);
        if (sent == -1) {
            std::perror("::send error");
            throw std::runtime_error("Send failed = -1");
        }
        else if (sent == 0) {
            throw std::runtime_error("Send failed = 0");
        }
        total_sent += sent;
    }
    Logger::log("Transport", "send() " + std::to_string(total_sent) + " bytes via TCP");
    Logger::log("Internet", "dst=127.0.0.1 proto=TCP");
    Logger::log("Network Access", "frame sent to network interface");
}

std::optional<MessageEx> Messenger::recvMsg(int fd) {
    std::string recv_str;
    char c;

    Logger::log("Network Access", "frame received via network interface");
    Logger::log("Internet", "src=127.0.0.1 dst=127.0.0.1 proto=TCP");

    while (true) {
        int received = ::recv(fd, &c, 1, 0);
        if (received == -1) {
            if (errno == ECONNRESET) return std::nullopt;
            std::perror("::recv error");
            throw std::runtime_error("Recv error");
        }
        if (received == 0) {
            return std::nullopt;
        }
        if (c == '\n') break;
        recv_str += c;
    }

    if (recv_str.empty()) return std::nullopt;

    MessageEx msg = json::parse(recv_str);
    
    Logger::log("Transport", 
        std::string("recv() ")  + std::to_string(recv_str.size()) + " bytes via TCP");
    
    return msg;
}

MessageEx stringToMsg(const std::string& str, MessageType type) {
    MessageEx msg;
    std::memset(msg.payload, 0, MAX_PAYLOAD);

    msg.length = str.size();
    msg.type = type;
    msg.msg_id = ++Id_Count;
    msg.timestamp = std::time(nullptr);
    
    std::memset(msg.sender, 0, MAX_NAME);
    std::memset(msg.receiver, 0, MAX_NAME);
    std::memcpy(msg.payload, str.data(), str.size());

    Logger::log("Application", "serialize Message -> type=" + std::to_string(type));
    return msg;
}

std::string msgToString(const MessageEx& msg) {
    Logger::log("Application", "deserialize Message -> type=" + std::to_string(msg.type));
    return msg.payload;
}

std::pair<std::string, std::string> convertToNick_Msg(const std::string& input) {
    std::istringstream iss(input);
    std::string command_str, nickname;
    
    if (!(iss >> command_str >> nickname)) {
        return {};
    }

    std::string msg;
    std::getline(iss, msg);
    
    if (!msg.empty() && msg.front() == ' ') {
        msg.erase(0, 1);
    }

    return {nickname, msg};
}

void printTextMessage(const MessageEx& msg) {
    std::cout << "\r" << formatTimestamp(msg.timestamp) << \
        "[id=" << msg.msg_id << "]" << \
        "[from " << msg.sender << "]: " << \
        msgToString(msg) << \
        "\n> " << std::flush;
}

void printPrivateMessage(const MessageEx& msg) {
   std::cout << "\r" << formatTimestamp(msg.timestamp) << \
        "[PRIVATE]" << \
        "[id=" << msg.msg_id << "]" << \
        "[" << msg.sender << " -> " << msg.receiver << "]: " << \
        msgToString(msg) << \
        "\n> " << std::flush;
}

void printOfflineMessage(const MessageEx& msg) {
    std::cout << "\r[" << formatTimestamp(msg.timestamp) << "]"
            << "[id=" << msg.msg_id << "]"
            << "[OFFLINE]"
            << "[" << msg.sender << " . " << msg.receiver << "]: "
            << std::string(msg.payload).substr(10)
            << "\n> " << std::flush;
}

std::string jsonToString(const json& j) {
    return j.dump();
}
