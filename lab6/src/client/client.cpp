#include "client.hpp"

#include <cstring>
#include <fstream>

Client::Client(const std::string &ip_addr, uint16_t port) :
    _ip_addr(ip_addr), _port(port)
{
}

void Client::connect() {
    _socket_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if (_socket_fd == -1) {
        throw std::runtime_error("Client socket creation failed");
    }

    _server_info.sin_family = AF_INET; 
    _server_info.sin_addr.s_addr = inet_addr(_ip_addr.c_str());
    _server_info.sin_port = htons(_port); 

    if (::connect(_socket_fd, (struct sockaddr *)&_server_info, sizeof(_server_info)) == -1) {
        throw std::runtime_error("Connection with the server failed");
    }

    std::cout << "Connected\n";
}

void Client::sendHello(const MessageEx& msg) {
    Client::rawSend(msg);
    Logger::log("Transport", "send() msg_id=" + std::to_string(msg.msg_id));
}

void Client::recvWelcome() {
    auto welcome_msg = Client::rawRecv();

    if (!welcome_msg.has_value()) {
        throw std::runtime_error("Nullopt welcome msg");
    }
    if (welcome_msg->type != MSG_WELCOME) {
        throw std::runtime_error("Unexpected msg type: " 
            + std::to_string(welcome_msg->type) 
            + " (instead of MSG_WELCOME)");
    }

    std::cout << msgToString(welcome_msg.value()) << std::endl;
}

void Client::startAckReader() {
    _ack_reader = std::thread([this]() {
        while (_socket_fd != -1) {
            auto msg = Client::rawRecv();
            if (!msg.has_value()) break;

            if (msg->type == MSG_ACK) {
                Logger::log("Application", 
                    std::string("client ack_reader got ACK for msg_id=") 
                    + std::to_string(msg->msg_id)
                );
                _messenger.notifyACK(msg->msg_id);
            } 
            else {
                _inbox.enqueue(msg.value());
            }
        }
    });
}

void Client::notifyAck(uint32_t expected_msg_id) {
    _messenger.notifyACK(expected_msg_id);
}

void Client::registerPing(uint32_t msg_id) {
    std::scoped_lock lock(_ping_mtx);
    _ping_sent[msg_id] = std::chrono::steady_clock::now();
}

void Client::registerPong(uint32_t msg_id) {
    std::scoped_lock lock(_ping_mtx);
    auto it = _ping_sent.find(msg_id);
    if (it == _ping_sent.end()) return;

    auto now = std::chrono::steady_clock::now();
    double rtt = std::chrono::duration<double, std::milli>(now - it->second).count();
    _ping_rtt[msg_id] = rtt;
}


void Client::auth(MessageEx msg) {
    msg.type = MSG_AUTH;

    Client::rawSend(msg);

    auto auth_msg = Client::rawRecv();

    if (!auth_msg.has_value()) {
        throw std::runtime_error("Nullopt auth msg");
    }
    if (auth_msg->type != MSG_AUTH) {
        throw std::runtime_error("Unexpected msg type: " 
            + std::to_string(auth_msg->type) 
            + " (instead of MSG_AUTH)");
    }
    if (auth_msg->type == MSG_ERROR) {
        throw std::runtime_error("Auth error: " +  msgToString(auth_msg.value()));
    }

    Logger::log("Session", "authentication success");
}

MessageEx Client::enterNickname() {
    std::cout << "Enter nickname: " << std::endl;
    std::string nickname_str;
    std::getline(std::cin, nickname_str);

    _nickname = nickname_str;

    return stringToMsg(nickname_str, MSG_HELLO);
}

void Client::close() {
    ::close(_socket_fd);
    _socket_fd = -1;
}

void Client::shutdown() {
    ::shutdown(_socket_fd, SHUT_WR);
    _socket_fd = -1;
}

void Client::send(const MessageEx& msg) {
    _messenger.sendMsg(msg, _socket_fd);
}

std::optional<MessageEx> Client::recv() {
    return _inbox.pop();
}

void Client::printPingResults(std::vector<PingStats>& stats) {
    int sent     = stats.size();
    int received = 0;
    double rtt_sum    = 0;
    double jitter_sum = 0;
    double prev_rtt   = -1;

    for (auto& s : stats) {
        auto rtt = getRtt(s.msg_id);
        if (rtt.has_value()) {
            ++received;
            rtt_sum += rtt.value();
            if (prev_rtt >= 0) {
                jitter_sum += std::abs(rtt.value() - prev_rtt);
            }
            prev_rtt = rtt.value();
            s.rtt_ms = rtt.value();
        }
    }

    double rtt_avg    = received > 0 ? rtt_sum / received : 0;
    double jitter_avg = received > 1 ? jitter_sum / (received - 1) : 0;
    double loss       = (sent - received) * 100.0 / sent;

    std::cout << "\nRTT avg : " << rtt_avg    << " ms\n"
              << "Jitter  : " << jitter_avg << " ms\n"
              << "Loss    : " << loss        << " %\n\n";

    saveNetDiag(rtt_avg, jitter_avg, loss, stats);
}

void Client::saveNetDiag(double rtt_avg, double jitter_avg, double loss,
                          const std::vector<PingStats>& stats) {
    std::string filename = "net_diag_" + _nickname + ".json";
    std::ofstream file(filename);
    if (!file.is_open()) return;

    json j;
    j["nickname"]    = _nickname;
    j["rtt_avg_ms"]  = rtt_avg;
    j["jitter_ms"]   = jitter_avg;
    j["loss_pct"]    = loss;
    j["timestamp"]   = std::time(nullptr);

    json packets = json::array();
    for (const auto& s : stats) {
        json p;
        p["msg_id"]  = s.msg_id;
        p["rtt_ms"]  = s.rtt_ms.value_or(-1);
        p["lost"]    = !s.rtt_ms.has_value();
        packets.push_back(p);
    }
    j["packets"] = packets;

    file << j.dump(4) << "\n";
    std::cout << "Saved to " << filename << "\n";
}

std::optional<double> Client::getRtt(uint32_t msg_id) {
    std::scoped_lock lock(_ping_mtx);
    auto it = _ping_rtt.find(msg_id);
    if (it == _ping_rtt.end()) return std::nullopt;
    return it->second;
}

std::string Client::getFormattedIpPort() const {
    std::string res;
    res.resize(20);
    std::sprintf(res.data(), "[%s:%d] ", _ip_addr.data(), _port);

    return res;
}

std::string Client::getNickname() const {
    return _nickname;
}

void Client::rawSend(const MessageEx& msg) {
    json j = msg;
    std::string j_str = j.dump() + "\n";

    _messenger.rawSend(_socket_fd, j_str);
}

std::optional<MessageEx> Client::rawRecv() {
    std::string recv_str;
    ssize_t received = _messenger.rawRecv(_socket_fd, recv_str);

    if (received <= 0) return std::nullopt; 
    if (recv_str.empty()) return std::nullopt;

    MessageEx msg = json::parse(recv_str);

    return msg;
}

void Client::reset() {
    _inbox.stop();
    if (_ack_reader.joinable()) {
        _ack_reader.join();
    }

    Client::close();
}
