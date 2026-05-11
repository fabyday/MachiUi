#include "ActionRegistry.h"
#include "ServiceRegistry.h"

#include <utility>

void ActionRegistry::onInit(ServiceProvider *provider)
{
}

void ActionRegistry::registerAction(const std::string &name, ActionHandler handler)
{
    if (name.empty() || !handler)
    {
        return;
    }

    handlers[name] = std::move(handler);
}

void ActionRegistry::unregisterAction(const std::string &name)
{
    handlers.erase(name);
}

bool ActionRegistry::hasAction(const std::string &name) const
{
    return handlers.find(name) != handlers.end();
}

ActionResult ActionRegistry::invoke(const std::string &name, const std::string &payloadJson) const
{
    auto it = handlers.find(name);
    if (it == handlers.end())
    {
        return ActionResult::Failure("Action is not registered: " + name);
    }

    return it->second(ActionRequest{name, payloadJson.empty() ? "null" : payloadJson});
}

REGISTER_UI_COMPONENT(ActionRegistry, ServicePhase::Logic);
