#pragma once
#include <iostream>

class Logger {
    Logger() = default;
public:
    static void log(
        int layer_num, 
        const std::string& layer_name,
        const std::string& message
    );
};