#include "ScriptManager.h"
#include "../Core/SceneManager.h"
#include "quickjs.h" // 여기서만 인클루드
#include "../Core/UiEngine.h"
#include "../Core/IFileLoader.h"
#include <iostream>
#include "../Core/Element.h"
#include "NativeBinder.h"
#include "ClassRegistry.h"

#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
// C++20 이상
#define MACHI_JS_CFUNC_DEF(name, length, func1)                                                        \
    {                                                                                                  \
        name,                                                                                          \
            .prop_flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE,                                     \
            .def_type = JS_DEF_CFUNC,                                                                  \
            .magic = 0,                                                                                \
            .u = {.func = {(int16_t)(length), JS_CFUNC_generic, {.generic = (JSCFunction *)(func1)}} } \
    }
#else
// C++17 이하:
#define MACHI_JS_CFUNC_DEF(name, length, func1)                                 \
    {                                                                           \
        name,                                                                   \
            JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE,                            \
            JS_DEF_CFUNC,                                                       \
            0,                                                                  \
        {                                                                       \
            {                                                                   \
                (int16_t)(length), JS_CFUNC_generic, { (JSCFunction *)(func1) } \
            }                                                                   \
        }                                                                       \
    }
#endif

// foward decl
struct
    ScriptManager::Impl
{
    JSRuntime *runtime;
    JSContext *context;
    Impl(ScriptManager *manager);
    ~Impl();
    void run(const std::string &code, const std::string &name, bool isModule);
    void handleException();
};
static void register_native_method(JSContext *ctx);
static JSModuleDef *js_module_loader(JSContext *ctx, const char *module_name, void *opaque);
static JSValue js_console_log(JSContext *ctx, JSValueConst func, int argc, JSValueConst *argv);
static char *js_module_normalize(JSContext *ctx, const char *module_referrer,
                                 const char *module_name, void *opaque);

// --- QuickJS와 직접 소통하는 내부 구현체 ---

ScriptManager::Impl::Impl(ScriptManager *manager)
{
    runtime = JS_NewRuntime();
    context = JS_NewContext(runtime);

    // register Moudle Loader Function
    JS_SetModuleLoaderFunc(runtime, js_module_normalize, js_module_loader, manager);
    JSValue globalObj = JS_GetGlobalObject(context);
    JSValue console = JS_NewObject(context);
    JS_SetPropertyStr(context, console, "log", JS_NewCFunction(context, js_console_log, "log", 1));
    JS_SetPropertyStr(context, globalObj, "console", console);
    register_native_method(context);

    // set opaque data for module loader
    JS_SetContextOpaque(context, manager);
    JS_FreeValue(context, globalObj);
}

ScriptManager::Impl::~Impl()
{
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
}

void ScriptManager::Impl::run(const std::string &code, const std::string &name, bool isModule)
{
    int flags = isModule ? JS_EVAL_TYPE_MODULE : JS_EVAL_TYPE_GLOBAL;
    JSValue result = JS_Eval(context, code.c_str(), code.length(), name.c_str(), flags);

    if (JS_IsException(result))
    {
        this->handleException();
    }
    else
    {
        // 결과값이 필요하다면 여기서 처리 (지금은 단순 출력)
        const char *str = JS_ToCString(context, result);
        if (str)
        {
            std::cout << "[JS Result] " << str << std::endl;
            JS_FreeCString(context, str);
        }
    }

    // 비동기 작업(Promise 등)이 남아있다면 실행
    JSContext *ctx_pending;
    while (JS_ExecutePendingJob(runtime, &ctx_pending) > 0)
    {

        std::cout << "Test" << std::endl;
    }

    /// TEST CODE
    // if (JS_IsPromise(result))
    // {
    //     // Promise의 상태를 강제로 확인 (0: Pending, 1: Fulfilled, 2: Rejected)
    //     // ※ 주의: QuickJS 버전에 따라 함수명이 다를 수 있으니 확인 필요
    //     int state = JS_PromiseState(context, result);
    //     if (state == 2)
    //     { // Rejected
    //         JSValue reason = JS_PromiseResult(context, result);
    //         const char *msg = JS_ToCString(context, reason);
    //         std::cout << msg << std::endl;
    //         JS_FreeCString(context, msg);
    //         JS_FreeValue(context, reason);
    //     }
    //     else if (state == 0)
    //     {
    //         std::cout << "JS Promise is still PENDING. Module loading might be stuck." << std::endl;
    //     }
    // }

    JS_FreeValue(context, result);
}

// 헬퍼: 예외 처리 로직을 Impl 안으로 격리
void ScriptManager::Impl::handleException()
{
    JSValue exception = JS_GetException(context);
    const char *str = JS_ToCString(context, exception);
    if (str)
    {
        std::cerr << "[JS Error] " << str << std::endl;
        JS_FreeCString(context, str);
    }
    JS_FreeValue(context, exception);
}

// --- 모듈 로더 콜백 (Static) ---
// static 함수이므로 파일 외부로 노출되지 않음
static JSModuleDef *js_module_loader(JSContext *ctx, const char *module_name, void *opaque)
{
    auto *self = static_cast<ScriptManager *>(opaque);
    auto *loader = self->GetFileLoader();
    if (!loader)
        return nullptr;

    auto code = loader->readFile(module_name);
    if (!code)
        return nullptr;

    // 모듈 컴파일
    JSValue func_val = JS_Eval(ctx, code->c_str(), code->length(), module_name,
                               JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(func_val))
        return nullptr;

    JSModuleDef *m = (JSModuleDef *)JS_VALUE_GET_PTR(func_val);
    JS_FreeValue(ctx, func_val);
    return m;
}

/**
 * Module path normalize function
 */
static char *js_module_normalize(JSContext *ctx, const char *module_referrer,
                                 const char *module_name, void *opaque)
{
    auto *self = static_cast<ScriptManager *>(opaque);
    auto *loader = self->GetFileLoader();
    if (!loader)
        return nullptr;

    auto resolved = loader->resolvePath(module_referrer, module_name);

    if (!resolved.has_value())
    {
        // JS 레벨에서 에러를 던지게 만들 수 있습니다.
        JS_ThrowReferenceError(ctx, "Could not resolve module path: '%s' from '%s'",
                               module_name, module_referrer);
        return nullptr; // nullptr를 반환하면 QuickJS가 예외 상태로 진입합니다.
    }

    // 성공 시 메모리 할당 후 반환
    char *res = (char *)js_malloc(ctx, resolved->size() + 1);
    if (res)
    {
        memcpy(res, resolved->c_str(), resolved->size() + 1);
    }
    return res;
}

static JSValue js_console_log(JSContext *ctx, JSValueConst func, int argc, JSValueConst *argv)
{

    for (int i = 0; i < argc; i++)
    {
        const char *str = JS_ToCString(ctx, argv[i]);
        if (str)
        {
            std::cout << "[JS Console] " << str << std::endl;
            JS_FreeCString(ctx, str);
        }
    }
    return JS_UNDEFINED;
}

static JSValue js_create_root(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv)
{
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (!sm || !sm->getSceneManager())
    {
        return JS_EXCEPTION;
    }
    const char *containerIdStr = JS_ToCString(ctx, argv[0]);
    uint64_t containerId = std::stoull(containerIdStr);
    JS_FreeCString(ctx, containerIdStr);

    uint64_t sceneGraphId = sm->getSceneManager()->createSceneGraph("DefaultScene");
    SceneGraph *graph = sm->getSceneManager()->getSceneGraph(sceneGraphId);
    sm->createOrGetExecutionContext("DefaultScene")->defaultSceneGraph = graph;

    // Root 객체 생성
    JSClassID classId = ClassRegistry::getOrCreateClassID(ctx, "Root");
    JSValue rootObj = JS_NewObjectClass(ctx, classId);
    JS_SetPropertyStr(ctx, rootObj, "_containerId", JS_NewBigUint64(ctx, containerId));
    return rootObj;
}

// create element like div, span, etc.
static JSValue js_create_element(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv)
{
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    // 2. 만약의 상황을 대비한 안전장치
    if (!sm || !sm->getSceneManager())
    {
        return JS_EXCEPTION;
    };
    const char *type = JS_ToCString(ctx, argv[0]);
    JSClassID classId = ClassRegistry::getOrCreateClassID(ctx, type);
    JSValue object = JS_NewObjectClass(ctx, classId);

    Element *elementPtr = sm->getSceneManager()->createElement(type);
    if (!elementPtr)
    {
        std::cerr << "Failed to create element of type: " << type << std::endl;
        JS_FreeCString(ctx, type);
        return JS_EXCEPTION; // 요소 생성 실패 시 예외 처리
    }
    JS_SetOpaque(object, elementPtr);
    std::cout << "[JS Native] Create Element of type: " << type << " with ID: " << elementPtr->getId() << std::endl;

    JS_FreeCString(ctx, type);
    return object;
}

static JSValue js_root_render(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv);

static JSValue js_append_child(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv);

static JSValue js_element_set_style(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv);

// 1. Element용 함수 (공통)
static const JSCFunctionListEntry element_funcs[] = {
    MACHI_JS_CFUNC_DEF("appendChild", 1, js_append_child),
    MACHI_JS_CFUNC_DEF("setStyle", 2, js_element_set_style),
};

// 2. Root 전용 함수 (차별점)
static const JSCFunctionListEntry root_funcs[] = {
    MACHI_JS_CFUNC_DEF("render", 1, js_root_render),
};

static JSValue js_root_render(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv)
{
    JSValue containerValue = JS_GetPropertyStr(ctx, this_val, "_containerId");
    uint64_t containerId = 0;
    uint64_t childId;
    JS_ToBigUint64(ctx, &childId, argv[0]);

    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (!sm || !sm->getSceneManager())
    {
        return JS_EXCEPTION;
    }

    SceneManager *sceneManager = sm->getSceneManager();
    SceneGraph *graph = sceneManager->getSceneGraph(containerId);
    sceneManager->destroyAllChildren(containerId);
    return JS_UNDEFINED;
}

static JSValue js_append_child(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv)
{
    uint64_t parentId = 0, childId = 0;
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (!sm || !sm->getSceneManager())
    {
        return JS_EXCEPTION;
    }
    SceneManager *sceneManager = sm->getSceneManager();
    Element *parent = sceneManager->getElement(parentId);
    Element *child = sceneManager->getElement(childId);

    if (!parent && !child)
    {
        return JS_ThrowInternalError(ctx, "Element not found. ID Might be invalid");
    }
    parent->appendChild(child);
    // GlobalPool에서 두 객체를 찾아 트리로 연결
    // 만약 parent가 특정 Scene에 속해있다면, 자연스럽게 child도 그 Scene의 렌더링 대상이 됨

    return JS_UNDEFINED;
}

static JSValue js_element_set_style(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv)
{

    return JS_UNDEFINED;
}

/// @brief  explicitly Destroy Element
/// @param ctx
/// @param this_val
/// @param argc
/// @param argv
/// @return
static JSValue js_destroy_element(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    int64_t ptr;
    JS_ToInt64(ctx, &ptr, argv[0]);

    Element *element = reinterpret_cast<Element *>(ptr);
    delete element; // 여기서 실제 메모리 해제

    return JS_UNDEFINED;
}

// Binding Default Elements (Root, Text, Image, etc.)
void register_default_elements(JSContext *ctx)
{
    GenericBinder({
                      "Element",
                      element_funcs,
                      sizeof(element_funcs) / sizeof(JSCFunctionListEntry),
                      nullptr // 부모 클래스 없음
                  })
        .Bind(ctx);

    GenericBinder({
                      "Root",
                      root_funcs,
                      sizeof(root_funcs) / sizeof(JSCFunctionListEntry),
                      "Element" // Root는 Element를 상속
                  })
        .Bind(ctx);
}

// 네이티브 메서드 등록 함수
void register_native_method(JSContext *ctx)
{
    JSValue native = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, native, "createElement", JS_NewCFunction(ctx, js_create_element, "createElement", 2));
    JS_SetPropertyStr(ctx, native, "createRoot", JS_NewCFunction(ctx, js_create_root, "createRoot", 2));
    register_default_elements(ctx);
    // appendChild, updateProp 등도 동일하게 등록...
    JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "MachiNative", native);
}