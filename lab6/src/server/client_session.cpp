#include "client_session.hpp"

#include "messaging/messaging.hpp"
#include "logger/logger.hpp"

#include <cstring>
#include <algorithm>

ClientSession::ClientSession(int conn_fd, sockaddr_in client_info)
    : _conn_fd(conn_fd), _client_info(client_info)
{
}

ClientSession::~ClientSession() {
    ClientSession::close();
    if (_ack_reader.joinable()) {
        _ack_reader.join();
    }
}

ClientSession::ClientSession(ClientSession&& other) {
    if (other.isActive()) { 
        _client_info = other._client_info;
        _conn_fd = other._conn_fd;
        
        other._conn_fd = -1;
    }
}

ClientSession& ClientSession::operator=(ClientSession&& other) {
    if (this == &other) {
        return *this;
    }
    if (this->isActive()) {
        this->close();
    }
    if (other.isActive()) {    
        _client_info = std::move(other._client_info);
        _conn_fd = other._conn_fd;
        
        other._conn_fd = -1;
    }
    return *this;
}

void ClientSession::recvHello() {
    auto nickname_msg = ClientSession::rawRecv();
    nickname = msgToString(nickname_msg.value());

    std::cout << "User [" << nickname << "] connected" << std::endl;
    
    if (!nickname_msg.has_value()) {
        throw std::runtime_error("Nullopt hello msg");
    }
    std::cout << "Hello " << nickname << std::endl;

}

void ClientSession::sendWelcome(uint16_t port) {
    std::string ip_str = inet_ntoa(_client_info.sin_addr);
    std::string port_str = std::to_string(port);
    std::string welcome_str = "Welcome " + ip_str + ":" + port_str;

    MessageEx msg = stringToMsg(welcome_str, MSG_WELCOME);
    
    ClientSession::rawSend(msg);
}

void ClientSession::sendPong() {
    MessageEx msg = stringToMsg("PONG", MSG_PONG);
    ClientSession::send(msg);
}

void ClientSession::send(const MessageEx& msg, int fd) {
    std::scoped_lock lock(_mtx);

    if (fd == -1) fd = _conn_fd;
    _messenger.sendMsg(msg, fd);
}

std::optional<MessageEx> ClientSession::recv() {
    return _inbox.pop();
}

void ClientSession::close() {
    if (isActive()) {
        ::close(_conn_fd);
        _conn_fd = -1;
    }
    _inbox.stop();
}

void ClientSession::rawSend(const MessageEx& msg, int fd) {
    std::scoped_lock lock(_mtx);

    if (fd == -1) fd = _conn_fd;

    json j = msg;
    std::string j_str = j.dump() + "\n";

    _messenger.rawSend(fd, j_str);
}

std::optional<MessageEx> ClientSession::rawRecv() {
    std::string recv_str;
    ssize_t received = _messenger.rawRecv(_conn_fd, recv_str);

    if (received <= 0) return std::nullopt; 
    if (recv_str.empty()) return std::nullopt;

    MessageEx msg = json::parse(recv_str);

    return msg;
}

void ClientSession::auth() {
    auto auth_msg = ClientSession::rawRecv();
    if (!auth_msg.has_value()) {
        throw std::runtime_error("Nullopt auth msg");
    }
    if (auth_msg->type != MSG_AUTH) {
        throw std::runtime_error("Unexpected auth msg type: " + std::to_string(auth_msg->type));
    }

    if (std::string(auth_msg->payload).empty() || auth_msg->length == 0) {
        ClientSession::rawSend(stringToMsg("Empty nickname", MSG_ERROR));
        ClientSession::close();

        throw std::runtime_error("Empty nickname");
    }

    ClientSession::rawSend(stringToMsg("AUTH", MSG_AUTH));
    Logger::log("Application", "client authenticated");
}

void ClientSession::startAckReader() {
    _ack_reader = std::thread([this]() {
        try {
            while (isActive()) {
                
                auto msg = ClientSession::rawRecv();
                if (!msg.has_value()) break;

                if (msg->type == MSG_ACK) {
                    _messenger.notifyACK(msg->msg_id);
                } 
                else {
                    _inbox.enqueue(msg.value());
                }
            }
        }
        catch (const std::exception& ex) {
            std::clog << ex.what() << std::endl;
        }
    });
}

void ClientSession::sendAckFor(uint32_t msg_id) {
    MessageEx ack;
    std::memset(&ack, 0, sizeof(ack));
    ack.type   = MSG_ACK;
    ack.msg_id = msg_id;
    rawSend(ack);
}