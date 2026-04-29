#pragma once
#include <iostream>
#include <string>

class Logger {
    Logger() = default;
public:
    static void log(
        const std::string& layer_name,
        const std::string& message
    );
};