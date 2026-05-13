#include "net_emulator.hpp"

bool NetEmulator::apply(MessageEx& msg){
    if (_config.delay_ms > 0) {
        Logger::log("Transport", "[SIM] DELAY applied: " 
                    + std::to_string(_config.delay_ms) + " ms");

        std::this_thread::sleep_for(std::chrono::milliseconds(_config.delay_ms));
    }

    if (_config.drop > 0 && _dist(_rng) < _config.drop) {
        Logger::log("Transport", "[SIM] DROP (id=" 
                    + std::to_string(msg.msg_id) 
                    + ", rate=" + std::to_string(_config.drop) + ")");
        return false;
    }

    if (_config.corrupt > 0 && _dist(_rng) < _config.corrupt) {
        Logger::log("Transport", "[SIM] CORRUPT payload (id=" 
                    + std::to_string(msg.msg_id) + ")");
                    
        int payload_len = std::strlen(msg.payload);
        if (payload_len > 0) {
            std::uniform_int_distribution<int> byte_dist(0, payload_len - 1);
            msg.payload[byte_dist(_rng)] ^= 0xFF; // bite inverse
        }
    }

    return true;
}