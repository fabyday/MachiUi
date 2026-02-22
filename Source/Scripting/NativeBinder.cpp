#include "NativeBinder.h"
#include "quickjs.h"
#include "ClassRegistry.h"
void GenericBinder::Bind(void *ctx_ptr)
{
    JSContext *context = static_cast<JSContext *>(ctx_ptr);

    // Get Class ID
    JSClassID classId = ClassRegistry::getOrCreateClassID(context, meta.className);

    JSValue proto = JS_NewObject(context);

    JS_SetPropertyFunctionList(context, proto, meta.functions, meta.numFunctions);

    if (meta.parentClassName)
    {
        JSValue parentProto = ClassRegistry::getPrototype(meta.parentClassName);
        // set prototype chain for inheritance
        if (!JS_IsNull(parentProto))
        {
            JS_SetPrototype(context, proto, parentProto);
        }
    }

    ClassRegistry::RegisterPrototype(meta.className, proto);

    // 클래스 객체에 prototype 연결
    JS_SetClassProto(context, classId, proto);
}