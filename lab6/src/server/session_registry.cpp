#include "session_registry.hpp"

#include <cstring>
#include <fstream>
#include "time_formatting.cpp"

void SessionRegistry::appendHistory(const MessageEx& msg, bool delivered, bool is_offline) {
    std::ofstream file(_history_path, std::ios::app); 
    if (!file.is_open()) {
        std::cerr << "Cannot open history file\n";
        return;
    }
\
    static const std::unordered_map<int, std::string> type_names = {
        {MSG_TEXT,    "MSG_TEXT"},
        {MSG_PRIVATE, "MSG_PRIVATE"},
        {MSG_AUTH,    "MSG_AUTH"},
    };
    std::string type_str = "UNKNOWN";
    auto it = type_names.find(msg.type);
    if (it != type_names.end()) type_str = it->second;

    json record = {
        {"msg_id",     msg.msg_id},
        {"timestamp",  msg.timestamp},
        {"sender",     std::string(msg.sender)},
        {"receiver",   std::string(msg.receiver)},
        {"type",       type_str},
        {"text",       std::string(msg.payload)},
        {"delivered",  delivered},
        {"is_offline", is_offline}
    };

    file << record.dump() << "\n"; \
}

void SessionRegistry::markDelivered(uint32_t msg_id) {
    std::ifstream in(_history_path);
    if (!in.is_open()) return;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) { lines.push_back(line); continue; }
        try {
            json rec = json::parse(line);
            if (rec.value("msg_id", 0u) == msg_id) {
                rec["delivered"] = true;
                rec["is_offline"] = false;
                lines.push_back(rec.dump());
            } else {
                lines.push_back(line);
            }
        } catch (...) {
            lines.push_back(line);
        }
    }
    in.close();

    std::ofstream out(_history_path, std::ios::trunc);
    for (const auto& l : lines) out << l << "\n";
}

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
    appendHistory(msg, true, false);
}

void SessionRegistry::sendPrivate(const MessageEx& msg,
        const std::string& target_nickname, const std::string& sender_name) {
    std::scoped_lock lock(_mtx);

    auto it_fd = _nickname_to_fd.find(target_nickname);
    if (it_fd == _nickname_to_fd.end()) {
        OfflineMsg offline;
        std::memset(&offline, 0, sizeof(offline));
        std::memcpy(offline.sender,   sender_name.data(),    sizeof(offline.sender) - 1);
        std::memcpy(offline.receiver, target_nickname.data(), sizeof(offline.receiver) - 1);
        std::memcpy(offline.text,     msg.payload,            sizeof(offline.text) - 1);
        offline.timestamp = msg.timestamp;
        offline.msg_id    = msg.msg_id;

        _offline_queue[target_nickname].push(offline);

        Logger::log("Application", "receiver " + target_nickname + " is offline, message queued");
        return;
    }

    int target_fd = it_fd->second;
    auto it_session = _sessions.find(target_fd);
    if (it_session == _sessions.end()) return;

    MessageEx forward_msg = msg;
    std::memcpy(forward_msg.sender, sender_name.data(),
                std::min(sender_name.size(), (size_t)MAX_NAME - 1));

    it_session->second->send(forward_msg);
    appendHistory(forward_msg, true, false);
}

void SessionRegistry::deliverOffline(const std::string& nickname, ClientSession* session) {
    std::scoped_lock lock(_mtx);

    auto it = _offline_queue.find(nickname);
    if (it == _offline_queue.end() || it->second.empty()) {
        Logger::log("Application", "no offline messages for " + nickname);
        return;
    }

    auto& queue = it->second;
    while (!queue.empty()) {
        const OfflineMsg& offline = queue.front();

        std::string text = std::string("[OFFLINE] ") + offline.text;
        MessageEx msg = stringToMsg(text, MSG_PRIVATE);
        msg.msg_id    = offline.msg_id;
        msg.timestamp = offline.timestamp;
        std::memcpy(msg.sender,   offline.sender,   MAX_NAME);
        std::memcpy(msg.receiver, offline.receiver, MAX_NAME);

        session->send(msg);
        Logger::log("Application", "delivered offline message to " + nickname);
        
        markDelivered(offline.msg_id);
        queue.pop();
    }

    _offline_queue.erase(it);
}

void SessionRegistry::registerNickname(int fd, const std::string& nickname) {
    std::scoped_lock lock(_mtx);
    _nickname_to_fd[nickname] = fd;
}

std::string SessionRegistry::getUserList() {
    std::scoped_lock lock(_mtx);

    std::string result;

    for (const auto& [nickname, fd] : _nickname_to_fd) {
        result += nickname + "\n";
    }
    return result;
}

std::string SessionRegistry::getHistory(int n) {
    std::ifstream file(_history_path);
    if (!file.is_open()) return "[SERVER]: No history available\n";

    std::vector<json> records;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        try { records.push_back(json::parse(line)); }
        catch (...) {}
    }

    if (n > 0 && (size_t)n < records.size()) {
        records = std::vector<json>(records.end() - n, records.end());
    }

    std::string result;
    for (const auto& rec : records) {
        std::string type_str = rec.value("type", "");
        time_t ts = rec.value("timestamp", (time_t)0);
        uint32_t id = rec.value("msg_id", 0);
        std::string sender = rec.value("sender", "");
        std::string receiver = rec.value("receiver", "");
        std::string text = rec.value("text", "");
        bool is_offline = rec.value("is_offline", false);

        result += "[" + formatTimestamp(ts) + "][id=" + std::to_string(id) + "]";

        if (is_offline)
            result += "[OFFLINE][" + sender + " -> " + receiver + "]: ";
        else if (type_str == "MSG_PRIVATE")
            result += "[PRIVATE][" + sender + " -> " + receiver + "]: ";
        else
            result += "[" + sender + "]: ";

        result += text + "\n";
    }
    return result;
}
