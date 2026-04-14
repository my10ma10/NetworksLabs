#include "session_registry.hpp"

#include <cstring>

void SessionRegistry::add(ClientSession& session) {
    std::scoped_lock lock(_mtx);
    _sessions[session.fd()] = &session;
}

void SessionRegistry::remove(int fd) {
    std::scoped_lock lock(_mtx);

    for (auto it = _nickname_to_fd.begin(); it != _nickname_to_fd.end(); ) {
        if (it->second == fd) {
            it = _nickname_to_fd.erase(it);
        } else {
            ++it;
        }
    }

    _sessions.erase(fd);
}

void SessionRegistry::broadcast(const Message& msg, int sender_fd, const std::string& sender_name) {
    std::scoped_lock lock(_mtx);
    
    Message broadcast_msg = formatBroadcastMsg(msg, sender_name);

    for (auto& [fd, session] : _sessions) {
        if (fd == sender_fd) continue;
        try {
            session->send(broadcast_msg);
        } 
        catch (const std::exception& ex) {
            std::cerr << "Broadcast to fd=" << fd << " failed: " << ex.what() << "\n";
        }
    }
}

void SessionRegistry::sendPrivate(const Message& msg,
                                  const std::string& target_nickname,
                                  const std::string& sender_name) {
    std::scoped_lock lock(_mtx);

    auto it_fd = _nickname_to_fd.find(target_nickname);
    if (it_fd == _nickname_to_fd.end()) {
        _sessions[it_fd->second]->send({19, MSG_ERROR, "Nickname not found"});

        std::cerr << "Nickname not found" << std::endl;
        return;
    }

    int target_fd = it_fd->second;

    auto it_session = _sessions.find(target_fd);
    if (it_session == _sessions.end()) {
        _sessions[it_fd->second]->send({18, MSG_ERROR, "Session not found"});
        std::cerr << "Session not found" << std::endl;
        return;
    }

    Message private_msg = msg;
    std::string prefixed = "[" + sender_name + "]: " + msg.payload;

    std::memset(private_msg.payload, 0, MAX_PAYLOAD);
    std::memcpy(private_msg.payload, prefixed.data(),
                std::min(prefixed.size(), (size_t)MAX_PAYLOAD - 1));

    private_msg.length = prefixed.size();

    it_session->second->send(private_msg);
}

void SessionRegistry::registerNickname(int fd, const std::string& nickname) {
    std::scoped_lock lock(_mtx);
    _nickname_to_fd[nickname] = fd;
}

Message SessionRegistry::formatBroadcastMsg(const Message& msg, const std::string& sender_name) {
    Message broadcast_msg = msg;
    std::string prefixed = sender_name + ": " + msg.payload;

    std::memset(broadcast_msg.payload, 0, MAX_PAYLOAD);
    std::memcpy(broadcast_msg.payload, prefixed.data(), 
                std::min(prefixed.size(), (size_t)MAX_PAYLOAD - 1));

    broadcast_msg.length = prefixed.size();
    
    return broadcast_msg;
}
