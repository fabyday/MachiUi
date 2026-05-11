#pragma once

#include "IService.h"

#include <functional>
#include <string>
#include <unordered_map>

struct ActionRequest
{
    std::string name;
    std::string payloadJson;
};

struct ActionResult
{
    bool ok = true;
    std::string payloadJson = "null";
    std::string error;

    static ActionResult Success(std::string payload = "null")
    {
        ActionResult result;
        result.ok = true;
        result.payloadJson = std::move(payload);
        return result;
    }

    static ActionResult Failure(std::string message)
    {
        ActionResult result;
        result.ok = false;
        result.payloadJson = "null";
        result.error = std::move(message);
        return result;
    }
};

class ActionRegistry : public IService
{
public:
    using ActionHandler = std::function<ActionResult(const ActionRequest &)>;

    void onInit(ServiceProvider *provider) override;

    void registerAction(const std::string &name, ActionHandler handler);
    void unregisterAction(const std::string &name);
    bool hasAction(const std::string &name) const;
    ActionResult invoke(const std::string &name, const std::string &payloadJson) const;

private:
    std::unordered_map<std::string, ActionHandler> handlers;
};
