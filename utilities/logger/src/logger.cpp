#include "logger.hpp"
#include <iostream>
#include <ctime>
#include <string>

namespace hrd41
{


LoggerError::LoggerError(const std::string& msg_)
    : std::runtime_error(msg_)
{
}


Logger::Logger(const std::string& fileName)
    : m_log_level(ERROR), m_logfile(fileName.c_str(), std::ios::app)
{
    if (!m_logfile.is_open())
    {
        throw LoggerError("Failed to open log file");
    }
}

void Logger::SetLoggerLevel(LogLevel loglevel)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_log_level = loglevel;
}

void Logger::SetFileName(const std::string& fileName)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_logfile.is_open())
    {
        m_logfile.close();
    }

    m_logfile.open(fileName.c_str(), std::ios::app);
    if (!m_logfile.is_open())
    {
        throw LoggerError("Failed to open new log file");
    }
}

std::string Logger::GetTime()
{
    std::time_t now = std::time(nullptr);
    std::tm* tm_info = std::localtime(&now);

    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);

    return std::string(buffer);
}
std::string Logger::GetColor(LogLevel lvl_)
{
    switch (lvl_)
    {
        case ERROR: return COLOR_RED;
        case INFO:  return COLOR_BLUE;
        case DEBUG: return COLOR_YELLOW;
        default:    return COLOR_RESET;
    }
}
std::string Logger::GetMode(LogLevel lvl_)
{
    switch (lvl_)
    {
        case ERROR: return "[ERROR]";
        case INFO:  return "[INFO] ";
        case DEBUG: return "[DEBUG]";
        default:    return "[UNKNOWN]";
    }
}




} // namespace hrd41