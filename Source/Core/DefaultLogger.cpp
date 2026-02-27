#include "DefaultLogger.h"

#include <spdlog/spdlog.h>
#include <string_view>
#include <spdlog/fmt/bundled/args.h>

// IArgVisitor를 상속받아 fmt의 store에 데이터를 채워주는 클래스
class FmtStoreFiller : public IArgVisitor
{
public:
    fmt::dynamic_format_arg_store<fmt::format_context> store;
    void
    Visit(int val) override
    {
        store.push_back(val);
    }
    void Visit(double val) override
    {
        store.push_back(val);
    }
    void Visit(std::string_view val) override { store.push_back(val); }
};

class SpdlogFormatter : public IFormatter
{

public:
    SpdlogFormatter()
    {

        spdlog::set_level(spdlog::level::info);
    };
    virtual ~SpdlogFormatter() override = default;

    virtual std::string Format(const char *fmt, const std::vector<RawArg> &args) override;
};

std::string
SpdlogFormatter::Format(const char *fmt, const std::vector<RawArg> &args)
{
    FmtStoreFiller filler;
    for (auto arg : args)
    {
        arg.applier(arg.value, filler);
    }

    return fmt::vformat(fmt, filler.store);
}

DefaultLogger::DefaultLogger()
{
    SpdlogFormatter *spdfmt = new SpdlogFormatter();
    this->setFormatter(spdfmt);
}

void DefaultLogger::logImpl(Level lv, const std::string &msg)
{
    switch (lv)
    {
    case Level::DEBUG:
        spdlog::debug(msg);
        break;
    case Level::INFO:
    default:
        spdlog::info(msg);
        break;
    }
}

void NullLogger::logImpl(Level lv, const std::string &msg)
{
    // Null-logger Do NOTHING.
    return;
}
