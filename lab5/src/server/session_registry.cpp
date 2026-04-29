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

void SessionRegistry::broadcast(const MessageEx& msg, int sender_fd, const std::string& sender_name) {
    std::scoped_lock lock(_mtx);
    
    Logger::log("Application",  "broadcast message");
        MessageEx broadcast_msg = msg;

    std::memcpy(broadcast_msg.sender, sender_name.data(),
                std::min(sender_name.size(), (size_t)MAX_NAME - 1));


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

void SessionRegistry::sendPrivate(const MessageEx& msg,
        const std::string& target_nickname, const std::string& sender_name) {
    std::scoped_lock lock(_mtx);

    auto it_fd = _nickname_to_fd.find(target_nickname);
    if (it_fd == _nickname_to_fd.end()) {
        auto sender_fd_it = _nickname_to_fd.find(sender_name);
        if (sender_fd_it != _nickname_to_fd.end()) {
            _sessions[sender_fd_it->second]->send(
                stringToMsg("Nickname not found", MSG_ERROR));
        }
        return;
    }

    int target_fd = it_fd->second;
    auto it_session = _sessions.find(target_fd);
    if (it_session == _sessions.end()) return;

    MessageEx forward_msg = msg;
    std::memcpy(forward_msg.sender, sender_name.data(),
                std::min(sender_name.size(), (size_t)MAX_NAME - 1));

    it_session->second->send(forward_msg);
}

void SessionRegistry::registerNickname(int fd, const std::string& nickname) {
    std::scoped_lock lock(_mtx);
    _nickname_to_fd[nickname] = fd;
}
