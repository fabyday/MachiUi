#include "ILogger.h"

class DefaultLogger : public ILogger
{

    //
    struct Impl;

    struct Impl *pImpl;

public:
    virtual ~DefaultLogger() = default;
    virtual void LogInfo(const std::string &msg) = 0;
    virtual void LogWarn(const std::string &msg) = 0;
    virtual void LogError(const std::string &msg) = 0;
};