#pragma once
#include <memory>
#include <string>

// 1. 로그 동작만 정의하는 인터페이스
class ILogger
{
public:
    virtual ~ILogger() = default;
    virtual void LogInfo(const std::string &msg) = 0;
    virtual void LogWarn(const std::string &msg) = 0;
    virtual void LogError(const std::string &msg) = 0;
};

