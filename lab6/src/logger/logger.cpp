#include "logger.hpp"

void Logger::log(
    const std::string& layer_name, 
    const std::string& message)
{
    std::clog << "\r" << "[" << layer_name << \
        "] " << message << "\n> " << std::flush;
}