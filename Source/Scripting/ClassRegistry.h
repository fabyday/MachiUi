#include <quickjs.h>
#include <unordered_map>
#include <string>
class ClassRegistry
{

    struct ClassInfo
    {
        JSClassID classId;
        JSValue proto;
    };

    static std::unordered_map<std::string, ClassInfo>
        registry;

public:
    // Create Javascript Classes and Prototypes, and return the class ID
    static JSClassID getOrCreateClassID(JSContext *ctx, const char *name);

    static void RegisterPrototype(const char *name, JSValue proto);
    static JSValue getPrototype(const char *name);
};
