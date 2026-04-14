#include "messaging.hpp"
#include <cstring>
#include <sstream>
#include <errno.h>

void Messenger::sendMsg(const Message& msg, int fd){
    size_t total_sent = 0;
    const char* buf = reinterpret_cast<const char*>(&msg);

    while (total_sent < sizeof(msg)) {
        int sent = ::send(fd, buf + total_sent, sizeof(msg) - total_sent, 0);
        if (sent == -1) {
            std::perror("::send error");
            throw std::runtime_error("Send failed = -1");
        }
        else if (sent == 0) {
            throw std::runtime_error("Send failed = 0");
        }
        total_sent += sent;
    }

}

std::optional<Message> Messenger::recvMsg(int fd) {
    Message msg;
    std::memset(&msg, 0, sizeof(msg));

    size_t total_received = 0;
    
    while (total_received < MAX_PAYLOAD) {
        int recv_len = ::recv(
            fd, &msg + total_received, 
            sizeof(Message) - total_received, 
            0
        );

        if (recv_len == -1) {
            if (errno != ECONNRESET) {
                std::perror("::recv error");
                throw std::runtime_error("Recv error");
            }
        }
        else if (recv_len == 0) {
            return std::nullopt;
        }
        total_received += recv_len;
    }
    msg.payload[msg.length] = '\0';
    
    return msg;
}

Message stringToMsg(const std::string& str, MessageType type) {
    Message msg;
    std::memset(msg.payload, 0, MAX_PAYLOAD);

    msg.length = str.size();
    msg.type = type;
    std::memcpy(msg.payload, str.data(), str.size());

    return msg;
}

std::string convertToNick_Msg(const std::string& input) {
    std::istringstream iss(input);
    std::string command_str, nickname;
    
    if (!(iss >> command_str >> nickname)) {
        return "";
    }

    std::string msg;
    std::getline(iss, msg);
    
    if (!msg.empty() && msg.front() == ' ') {
        msg.erase(0, 1);
    }

    return nickname + ":" + msg;
}

std::pair<std::string, std::string> convertFromNick_Msg(const std::string& input) {
    std::size_t semicolonPos = input.find(":");

    std::string nickname = input.substr(0, semicolonPos);
    std::string message_str = input.substr(semicolonPos + 1); 

    return {nickname, message_str};
}
