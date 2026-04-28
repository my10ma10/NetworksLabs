#include "logger.hpp"

void Logger::log(
    int layer_num, 
    const std::string& layer_name, 
    const std::string& message)
{
    std::clog << "[Layer " << layer_num << " - " << layer_name << \
        "] " << message << std::endl;
}