#pragma once
#include <random>
#include <chrono>
#include <cstring>
#include <thread>

#include "defines.hpp"
#include "logger/logger.hpp"

struct NetConfig {
    int delay_ms  = 0;
    float drop    = 0.0f;
    float corrupt = 0.0f;
};

class NetEmulator {
    NetConfig _config;
    std::mt19937 _rng{std::random_device{}()};
    std::uniform_real_distribution<float> _dist{0.0f, 1.0f};

public:
    explicit NetEmulator(const NetConfig& config) : _config(config) {}
    bool apply(MessageEx& msg);
};