#include "logger.hpp"

void Logger::log(
    const std::string& layer_name, 
    const std::string& message)
{
    std::clog << "[" << layer_name << \
        "] " << message << std::endl;
}