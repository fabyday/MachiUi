#pragma once

#include "IFileLoader.h"

class DefaultFileLoader : public IFIleLoader
{
public:
    DefaultFileLoader();
    virtual ~DefaultFileLoader() = default;

    virtual void OnInit(UiEngine *engine);
    virtual void OnUpdate() override;
    virtual std::optional<std::string> resolvePath(const std::string &referrer, const std::string &name) override;

    std::optional<std::string> readFile(const std::string &path) override;
};