#include <ctime>
#include <string>
#include <iostream>

#include "defines.hpp"

inline std::string formatTimestamp(time_t timestamp) {
    struct tm* tm_info = std::localtime(&timestamp);
    
    char buf[MAX_TIME_STR];
    std::strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", tm_info);
    
    return std::string(buf);
}


