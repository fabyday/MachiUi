#pragma once
#include <optional>
#include <string>
#include "IComponent.h"

class IFIleLoader : public IComponent
{

public:
    virtual ~IFIleLoader() = default;

    virtual std::optional<std::string> readFile(const std::string &path) = 0;

    // referrer: 기준이 되는 파일 경로, name: 가져오려는 상대 경로
    virtual std::optional<std::string> resolvePath(const std::string &referrer, const std::string &name) = 0;
};