#include "messaging.hpp"
#include "../time_formatting.cpp"

#include <cstring>
#include <sstream>
#include <errno.h>
#include <iostream>

void Messenger::sendMsg(const MessageEx& msg, int fd) {
    json j = msg;
    std::string j_str = j.dump() + "\n";

    for (int attempt = 1; attempt <= MAX_ACK_RETRIES; ++attempt) {
        size_t total_send = rawSend(fd, j_str);

        Logger::log("Transport", "send() attempt " + std::to_string(attempt) 
                    + ", msg_id=" + std::to_string(msg.msg_id));

        if (waitACK(msg.msg_id)) {
            Logger::log("Application", "ACK received for msg_id=" 
                        + std::to_string(msg.msg_id)
                    );
            return;
        }

        Logger::log("Application", "ACK timeout, retrying... (" 
                    + std::to_string(attempt) + "/" + std::to_string(MAX_ACK_RETRIES) + ")"
                );
    }
    throw std::runtime_error("No ACK after " + std::to_string(MAX_ACK_RETRIES) + " attempts");
}

std::optional<MessageEx> Messenger::recvMsg(int fd) {
    Logger::log("Network Access", "frame received via network interface");
    Logger::log("Internet", "src=127.0.0.1 dst=127.0.0.1 proto=TCP");

    std::string recv_str;
    ssize_t received = rawRecv(fd, recv_str);

    if (received == -1) return std::nullopt; 
    if (recv_str.empty()) return std::nullopt;


    MessageEx msg = json::parse(recv_str);
    
    Logger::log("Transport", 
        std::string("recv() ")  + std::to_string(recv_str.size()) + " bytes via TCP");
    
    return msg;
}

void Messenger::notifyACK(uint32_t msg_id) {
    std::scoped_lock lock(_ack_mtx);
    std::clog << "[DEBUG] notifyACK called for msg_id=" << msg_id << "\n";
    _pending_acks[msg_id] = true;
    _ack_cv.notify_all();
}

void Messenger::sendACK(uint32_t msg_id, int fd) {
    MessageEx ack;
    std::memset(&ack, 0, sizeof(ack));
    ack.type   = MSG_ACK;
    ack.msg_id = msg_id;
    ack.length = 0;

    std::string ack_str = json(ack).dump() + "\n"; 
    rawSend(fd, ack_str);
}

bool Messenger::waitACK(uint32_t expected_msg_id) {
    std::unique_lock lock(_ack_mtx);

    bool received = _ack_cv.wait_for(lock, std::chrono::milliseconds(ACK_TIMEOUT_MS),
        [&] { return _pending_acks.count(expected_msg_id) > 0; });

    if (received) {
        _pending_acks.erase(expected_msg_id);
    }
    return received;
}

size_t Messenger::rawSend(int fd, std::string_view str) {
    const char* buf = str.data();
    size_t total_sent = 0;

    while (total_sent < str.size()) {
        int sent = ::send(fd, buf + total_sent, str.size() - total_sent, 0);
        if (sent == -1) {
            std::perror("::send error");
            throw std::runtime_error("Send failed = -1");
        }
        else if (sent == 0) {
            throw std::runtime_error("Send failed = 0");
        }
        total_sent += sent;
    }
    return total_sent;
}

ssize_t Messenger::rawRecv(int fd, std::string& str) {
    char c;
    
    ssize_t total_received = 0;
    while (true) {
        int received = ::recv(fd, &c, 1, 0);
        if (received == -1) {
            if (errno == ECONNRESET) return -1;
            std::perror("::recv error");
            throw std::runtime_error("Recv error");
        }
        if (received == 0) {
            return -1;
        }
        if (c == '\n') break;
        str += c;

        total_received += received;
    }
    return total_received;
}

timeval Messenger::initTimeVal() {
    struct timeval tv;
    tv.tv_sec = ACK_TIMEOUT_MS / 1000;
    tv.tv_usec = (ACK_TIMEOUT_MS % 1000) * 1000;
    return tv; 
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
