// ============================================================================
// logger.hpp
// What:  Singleton logger that writes to file with screen output option
// Why:   Centralized logging across entire system; 3 levels (ERROR/INFO/DEBUG)
//        control verbosity; thread-safe for concurrent logging
// How:   Logger::Write(msg, level, printToScreen) logs with timestamp, thread
//        ID; Singleton ensures single global instance accessible from anywhere
// ============================================================================

#ifndef __ILRD_LOGGER_H__
#define __ILRD_LOGGER_H__

#include <iostream>
#include <string>       // std::string
#include <mutex>        // std::mutex, mutex::lockgourd
#include <fstream>      // std::fstream
#include <stdexcept>    // std::runtime_error
#include <sstream>
#include <thread>

#include "singelton.hpp" // Singleton

namespace hrd41
{
namespace
{
std::string COLOR_RED("\033[31m");
std::string COLOR_BLUE("\033[34m");
std::string COLOR_YELLOW( "\033[33m");
std::string COLOR_RESET("\033[0m");
}
class Logger
{
public:
    //Log level is ERROR by defualt
    enum LogLevel
    {
        ERROR = 0,
        INFO = 1,
        DEBUG = 2
    };

    Logger(const Logger& o_) = delete;
    Logger& operator=(const Logger& o_) = delete;
    ~Logger() = default;

    template<typename... Args>
    void Write(const std::string& msg,
                LogLevel loglevel = LogLevel::ERROR,
                bool printToScreen = true,
                Args... args);

    void SetLoggerLevel(LogLevel loglevel);
    void SetFileName(const std::string& fileName);



private:
    explicit Logger(const std::string& fileName = "log.txt");

    LogLevel m_log_level;
    std::mutex m_mutex;
    std::fstream m_logfile;

    std::string GetTime();
    std::string GetMode(LogLevel lvl_);
    std::string GetColor(LogLevel lvl_);
    //friend classes
    friend class Singleton<Logger>;
    
};

class LoggerError : public std::runtime_error
{
public:
    explicit LoggerError(const std::string& msg_);
    ~LoggerError() = default;
private:
};


template<typename... Args>
void Logger::Write(const std::string& msg,
                   LogLevel loglevel,
                   bool printToScreen,
                   Args... args)
{
    if (loglevel > m_log_level)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    std::ostringstream oss;
    oss << GetTime() << " "
        << GetMode(loglevel) << std::this_thread::get_id() << " "
        << msg;

    ((oss << " " << args), ...);

    std::string finalMsg = oss.str();

    // Write to file (no color)
    m_logfile << finalMsg << std::endl;

    // Print to console with color
    if (printToScreen)
    {
        std::cout << GetColor(loglevel)
                  << finalMsg
                  << COLOR_RESET
                  << std::endl;
    }
}
}//hrd41;
#endif //__ILRD_LOGGER_H__