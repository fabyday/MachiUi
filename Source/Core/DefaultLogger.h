#include "ILogger.h"
#include <string_view>

class DefaultLogger : public ILogger
{

public:
    DefaultLogger();
    virtual ~DefaultLogger() = default;

    virtual void logImpl(Level lv, const std::string &msg) override;
    virtual void setLevel(Level lv) override;
};

class NullLogger : public ILogger
{
public:
    NullLogger() = default;
    virtual ~NullLogger() = default;
    virtual void logImpl(Level lv, const std::string &msg);
    virtual void setLevel(Level lv) override;
};