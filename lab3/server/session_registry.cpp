#include "session_registry.hpp"

#include <cstring>

void SessionRegistry::add(ClientSession& session) {
    std::scoped_lock lock(_mtx);
    _sessions[session.fd()] = &session;
}

void SessionRegistry::remove(int fd) {
    std::scoped_lock lock(_mtx);
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

Message SessionRegistry::formatBroadcastMsg(const Message& msg, const std::string& sender_name) {
    Message broadcast_msg = msg;
    std::string prefixed = sender_name + ": " + msg.payload;

    std::memset(broadcast_msg.payload, 0, MAX_PAYLOAD);
    std::memcpy(broadcast_msg.payload, prefixed.data(), 
                std::min(prefixed.size(), (size_t)MAX_PAYLOAD - 1));

    broadcast_msg.length = prefixed.size();
    
    return broadcast_msg;
}
