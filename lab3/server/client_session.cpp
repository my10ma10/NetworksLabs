#include "client_session.hpp"

#include <cstring>

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
    
    if (!nickname_msg.has_value()) {
        throw std::runtime_error("Nullopt hello msg");
    }
    _fd_nicknames_map[_conn_fd] = nickname_msg->payload;
    std::cout << "Hello " << nickname_msg->payload << std::endl;

}

void ClientSession::sendWelcome(uint16_t port) {
    std::string ip_str = inet_ntoa(_client_info.sin_addr);
    std::string port_str = std::to_string(port);
    std::string welcome_str = "Welcome " + ip_str + ":" + port_str;

    Message msg;
    msg.length = welcome_str.size();
    msg.type = MSG_WELCOME;

    std::memset(msg.payload, 0, MAX_PAYLOAD);
    std::memcpy(msg.payload, welcome_str.data(), welcome_str.size());
    
    ClientSession::send(msg);
}

void ClientSession::sendPong() {
    Message msg {5, MSG_PONG, "PONG"};
    ClientSession::send(msg);
}

void ClientSession::send(const Message& msg) {
    std::scoped_lock lock(_mtx);
    _messenger.sendMsg(msg, _conn_fd);
}

std::optional<Message> ClientSession::recv() {
    return _messenger.recvMsg(_conn_fd);
}

void ClientSession::close() {
    if (isActive()) {
        _fd_nicknames_map.erase(_conn_fd);
        ::close(_conn_fd);
        _conn_fd = -1;
    }
}

