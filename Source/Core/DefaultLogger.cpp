#include "DefaultLogger.h"

#include <spdlog/spdlog.h>

struct DefaultLogger::Impl
{
};

void DefaultLogger::LogInfo(const std::string &msg)
{
    spdlog::info("Welcome to spdlog!");
    spdlog::error("Some error message with arg: {}", 1);
}
