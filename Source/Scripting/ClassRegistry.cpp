#include "ClassRegistry.h"

std::unordered_map<std::string, ClassRegistry::ClassInfo> ClassRegistry::registry;

JSClassID ClassRegistry::getOrCreateClassID(JSContext *ctx, const char *name)
{
    auto it = registry.find(name);
    if (it != registry.end())
    {
        return it->second.classId;
    }

    JSClassID newId = 0;
    JS_NewClassID(JS_GetRuntime(ctx), &newId);
    // Create a new prototype object for this class
    JSClassDef classDef = {
        name,
        nullptr, // 필요에 따라 finalizer 설정
    };
    // Register the class with the runtime
    JS_NewClass(JS_GetRuntime(ctx), newId, &classDef);
    // Store the class info in the registry
    registry[name] = {newId, JS_UNDEFINED};

    return newId;
}

void ClassRegistry::RegisterPrototype(const char *name, JSValue proto)
{
    if (registry.find(name) != registry.end())
    {
        registry[name].proto = JS_DupValue(nullptr, proto);
    }
}

JSValue ClassRegistry::getPrototype(const char *name)
{
    if (registry.find(name) != registry.end())
    {
        return registry[name].proto;
    }
    return JS_NULL;
}
