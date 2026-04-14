#include "client_session.hpp"

#include "../messaging.hpp"

#include <cstring>
#include <algorithm>

ClientSession::ClientSession(int conn_fd, sockaddr_in client_info)
    : _conn_fd(conn_fd), _client_info(client_info)
{
}

ClientSession::~ClientSession() {
    ClientSession::close();
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
    auto nickname_msg = ClientSession::recv();
    nickname = nickname_msg.value().payload;

    std::cout << "User [" << nickname << "] connected" << std::endl;
    
    if (!nickname_msg.has_value()) {
        throw std::runtime_error("Nullopt hello msg");
    }
    std::cout << "Hello " << nickname_msg->payload << std::endl;

}

void ClientSession::sendWelcome(uint16_t port) {
    std::string ip_str = inet_ntoa(_client_info.sin_addr);
    std::string port_str = std::to_string(port);
    std::string welcome_str = "Welcome " + ip_str + ":" + port_str;

    Message msg = stringToMsg(welcome_str, MSG_WELCOME);
    
    ClientSession::send(msg);
}

void ClientSession::sendPong() {
    Message msg {5, MSG_PONG, "PONG"};
    ClientSession::send(msg);
}

void ClientSession::send(const Message& msg, int fd) {
    std::scoped_lock lock(_mtx);

    if (fd == -1) fd = _conn_fd;
    _messenger.sendMsg(msg, fd);
}

std::optional<Message> ClientSession::recv() {
    return _messenger.recvMsg(_conn_fd);
}

void ClientSession::close() {
    if (isActive()) {

        ::close(_conn_fd);
        _conn_fd = -1;
    }
}

void ClientSession::auth() {
    auto auth_msg = ClientSession::recv();
    if (!auth_msg.has_value()) {
        throw std::runtime_error("Nullopt auth msg");
    }
    if (auth_msg->type != MSG_AUTH) {
        throw std::runtime_error("Unexpected auth msg type: " + std::to_string(auth_msg->type));
    }

    if (std::string(auth_msg->payload).empty() || auth_msg->length == 0) {
        ClientSession::send({15, MSG_ERROR, "Empty nickname"});
        ClientSession::close();

        throw std::runtime_error("Empty nickname");
    }

    std::cout << "[Layer 5] authentication success" << std::endl;
    ClientSession::send({5, MSG_AUTH, "AUTH"});

}
