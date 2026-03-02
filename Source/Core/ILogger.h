#pragma once

#include <memory>
#include <string>
#include "IFormatter.h"

enum class Level
{
    INFO,
    DEBUG,
    TRACE,
    VERBOSE,
    CRITIAL,
    ERROR,
    WARN
};

// 1. 로그 동작만 정의하는 인터페이스
class ILogger
{
private:
    IFormatter *formatter;

public:
    virtual ~ILogger() = default;
    void setFormatter(IFormatter *formatter)
    {
        this->formatter = formatter;
    }
    IFormatter *getFormatter()
    {
        return this->formatter;
    }

    template <typename... Args>
    void Log(Level lv, const char *format, Args &&...args)
    {
        std::vector<RawArg> packed = {make_arg(args)...};
        std::string msg = this->formatter->Format(format, packed);
        this->logImpl(lv, msg);
    }
    void Log(Level lv, const char *msg)
    {
        this->logImpl(lv, msg);
    }

    template <typename... Args>
    void LogInfo(const char *format, Args &&...args)
    {
        this->Log(Level::INFO, format, args...);
    }

    void LogInfo(const std::string &msg)
    {
        this->Log(Level::INFO, msg.c_str());
    }

    template <typename... Args>
    void LogWarn(const char *format, Args &&...args)
    {
        this->Log(Level::WARN, format, args...);
    }

    void LogWarn(const std::string &msg)
    {
        this->Log(Level::WARN, msg.c_str());
    }

    template <typename... Args>
    void LogError(const char *format, Args &&...args)
    {
        this->Log(Level::ERROR, format, args...);
    }
    void LogError(const std::string &msg)
    {
        this->Log(Level::ERROR, msg.c_str());
    }
    template <typename... Args>
    void LogDebug(const char *format, Args &&...args)
    {
        this->Log(Level::DEBUG, format, args...);
    }
    void LogDebug(const std::string &msg)
    {
        this->Log(Level::DEBUG, msg.c_str());
    }

    virtual void logImpl(Level lv, const std::string &msg) = 0;
    virtual void setLevel(Level lv) = 0;
};
