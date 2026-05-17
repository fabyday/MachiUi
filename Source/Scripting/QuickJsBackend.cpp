#include "ScriptManager.h"
#include "../Core/SceneManager.h"
#include "quickjs.h" // 여기서만 인클루드
#include "../Core/UiEngine.h"
#include "../Core/IFileLoader.h"
#include <iostream>
#include "../Core/Element.h"
#include "NativeBinder.h"
#include "ClassRegistry.h"
#include "../Core/InputManager.h"
#include "../Core/ActionRegistry.h"
#include "../Core/NativeViewRegistry.h"
#include "../Core/LogManager.h"
#include "../Core/ILogger.h"
#include "../Core/Message.h"
#include "../Elements/NativeView.h"
#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwctype>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

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
static void register_native_method(JSContext *ctx, ScriptManager *manager);
static JSModuleDef *js_module_loader(JSContext *ctx, const char *module_name, void *opaque);
static JSValue js_console_log(JSContext *ctx, JSValueConst func, int argc, JSValueConst *argv);
static char *js_module_normalize(JSContext *ctx, const char *module_referrer,
                                 const char *module_name, void *opaque);
static void js_release_event_runtime(JSContext *ctx);

// --- QuickJS와 직접 소통하는 내부 구현체 ---

ScriptManager::Impl::Impl(ScriptManager *manager)
{
    runtime = JS_NewRuntime();
    context = JS_NewContext(runtime);

    // register Moudle Loader Function
    JS_SetModuleLoaderFunc(runtime, js_module_normalize, js_module_loader, manager);
    JS_SetContextOpaque(context, manager);
    JSValue globalObj = JS_GetGlobalObject(context);
    JSValue console = JS_NewObject(context);
    JS_SetPropertyStr(context, console, "log", JS_NewCFunction(context, js_console_log, "log", 1));
    JS_SetPropertyStr(context, globalObj, "console", console);
    register_native_method(context, manager);
    JS_FreeValue(context, globalObj);
}

ScriptManager::Impl::~Impl()
{
    js_release_event_runtime(context);
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

#if 0
static JSValue js_create_root_old(JSContext *ctx, JSValueConst this_val,
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

#endif

static JSValue js_create_root(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv)
{
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (!sm || !sm->getSceneManager())
    {
        return JS_EXCEPTION;
    }

    SceneManager *sceneManager = sm->getSceneManager();
    SceneGraph *graph = nullptr;

    ScriptExecutionContext *activeContext = sm->getActiveContext();
    if (activeContext != nullptr)
    {
        graph = activeContext->defaultSceneGraph;
    }

    if (graph == nullptr)
    {
        const char *sceneName = nullptr;
        if (argc > 0 && !JS_IsUndefined(argv[0]) && !JS_IsNull(argv[0]))
        {
            sceneName = JS_ToCString(ctx, argv[0]);
        }

        uint64_t sceneGraphId = sceneManager->createSceneGraph(sceneName != nullptr ? sceneName : "DefaultScene");
        if (sceneName != nullptr)
        {
            JS_FreeCString(ctx, sceneName);
        }
        graph = sceneManager->getSceneGraph(sceneGraphId);
    }

    if (graph == nullptr)
    {
        return JS_ThrowInternalError(ctx, "Failed to create scene graph");
    }

    if (graph->getRoot() == nullptr)
    {
        if (!sceneManager->createRoot(graph->getUid()))
        {
            return JS_ThrowInternalError(ctx, "Failed to create root element");
        }
    }

    return JS_NewBigUint64(ctx, graph->getRoot()->getUid());
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

    auto logManager = sm->getLogManger();
    ILogger *logger = logManager->getLogger();

    const char *type = JS_ToCString(ctx, argv[0]);
    MACHI_LOG_DEBUG(logger, "[{} : {}]", "create ", type);

    Element *elementPtr = sm->getSceneManager()->createElement(type);
    if (!elementPtr)
    {
        logger->LogError("Failed to create element of type: {}", type);
        JS_FreeCString(ctx, type);
        return JS_EXCEPTION; // 요소 생성 실패 시 예외 처리
    }

    logger->LogDebug("[JS Native] Create Element of type: {} with ID: {}", type, elementPtr->getId());
    JSValue result = JS_NewBigUint64(ctx, elementPtr->getUid());
    JS_FreeCString(ctx, type);
    return result;
}
////////////////////////////////////////////////////////////////////////////
// C++ Native Functions for quickJS
static JSValue js_root_render(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv);

static JSValue js_append_child(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv);
static JSValue js_insert_before(JSContext *ctx, JSValueConst this_val,
                                int argc, JSValueConst *argv);
static JSValue js_remove_child(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv);
static JSValue js_clear_children(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv);
static JSValue js_element_set_style(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv);
static JSValue js_update_props(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv);
static JSValue js_update_event_handler(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv);
static JSValue js_update_global_event_handler(JSContext *ctx, JSValueConst this_val,
                                              int argc, JSValueConst *argv);
static JSValue js_get_bounding_client_rect(JSContext *ctx, JSValueConst this_val,
                                           int argc, JSValueConst *argv);
// TODO detachChild()
static JSValue js_create_element(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv);

static JSValue js_create_text_node(JSContext *ctx, JSValueConst this_val,
                                   int argc, JSValueConst *argv);
static JSValue js_update_text(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv);
static JSValue js_is_network_enabled(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv);
static JSValue js_fetch_sync(JSContext *ctx, JSValueConst this_val,
                             int argc, JSValueConst *argv);
static JSValue js_invoke_action(JSContext *ctx, JSValueConst this_val,
                                int argc, JSValueConst *argv);

////////////////////////////////////////////////////////////////////////////

// 1. Element용 함수 (공통)
static const JSCFunctionListEntry element_funcs[] = {
    MACHI_JS_CFUNC_DEF("appendChild", 1, js_append_child),
    MACHI_JS_CFUNC_DEF("setStyle", 2, js_element_set_style),
};

// 2. Root 전용 함수 (차별점)
static const JSCFunctionListEntry root_funcs[] = {
    MACHI_JS_CFUNC_DEF("render", 1, js_root_render),
};

static bool js_read_element_id(JSContext *ctx, JSValueConst value, uint64_t &id)
{
    id = 0;
    return JS_ToBigUint64(ctx, &id, value) == 0;
}

struct JsEventRuntime
{
    JSRuntime *runtime = nullptr;
    JSContext *context = nullptr;
    ScriptManager *scriptManager = nullptr;
    bool subscribed = false;
    std::unordered_map<uint64_t, std::unordered_map<std::string, JSValue>> handlers;
    std::unordered_map<std::string, JSValue> globalHandlers;
    uint64_t focusedElementId = 0;
    uint64_t pointerCaptureElementId = 0;
    uint64_t pointerDownElementId = 0;
    float pointerDownX = 0.0f;
    float pointerDownY = 0.0f;
    float lastPointerX = 0.0f;
    float lastPointerY = 0.0f;
    bool pointerDown = false;
    bool dragging = false;
    bool nativePointerCaptured = false;
};

static std::unordered_map<JSContext *, std::unique_ptr<JsEventRuntime>> g_eventRuntimes;

static JsEventRuntime *find_event_runtime(JSContext *ctx)
{
    auto it = g_eventRuntimes.find(ctx);
    if (it == g_eventRuntimes.end())
    {
        return nullptr;
    }

    return it->second.get();
}

static JsEventRuntime *ensure_event_runtime(JSContext *ctx, ScriptManager *manager)
{
    if (ctx == nullptr)
    {
        return nullptr;
    }

    auto it = g_eventRuntimes.find(ctx);
    if (it != g_eventRuntimes.end())
    {
        return it->second.get();
    }

    auto runtime = std::make_unique<JsEventRuntime>();
    runtime->runtime = JS_GetRuntime(ctx);
    runtime->context = ctx;
    runtime->scriptManager = manager;
    auto *rawRuntime = runtime.get();
    g_eventRuntimes[ctx] = std::move(runtime);
    return rawRuntime;
}

static void js_release_event_runtime(JSContext *ctx)
{
    auto it = g_eventRuntimes.find(ctx);
    if (it == g_eventRuntimes.end())
    {
        return;
    }

    for (auto &elementHandlers : it->second->handlers)
    {
        for (auto &handler : elementHandlers.second)
        {
            JS_FreeValue(ctx, handler.second);
        }
    }
    for (auto &handler : it->second->globalHandlers)
    {
        JS_FreeValue(ctx, handler.second);
    }

    g_eventRuntimes.erase(it);
}

static void log_js_exception(JSContext *ctx)
{
    JSValue exception = JS_GetException(ctx);
    const char *message = JS_ToCString(ctx, exception);
    if (message != nullptr)
    {
        std::cerr << "[JS Event Error] " << message << std::endl;
        JS_FreeCString(ctx, message);
    }
    JS_FreeValue(ctx, exception);
}

static bool element_has_handler(JsEventRuntime *runtime, uint64_t elementId, const std::string &eventName)
{
    if (runtime == nullptr)
    {
        return false;
    }

    auto elementIt = runtime->handlers.find(elementId);
    if (elementIt == runtime->handlers.end())
    {
        return false;
    }

    return elementIt->second.find(eventName) != elementIt->second.end();
}

static std::optional<std::string> get_element_string_attribute(Element *element, const char *key)
{
    if (element == nullptr)
    {
        return std::nullopt;
    }

    if (const auto *value = element->getAttribute(key))
    {
        if (const auto *stringValue = std::get_if<std::string>(value))
        {
            return *stringValue;
        }
    }

    return std::nullopt;
}

static float get_element_float_attribute(Element *element, const char *key, float fallback)
{
    if (element == nullptr)
    {
        return fallback;
    }

    const auto *value = element->getAttribute(key);
    if (value == nullptr)
    {
        return fallback;
    }

    if (const auto *intValue = std::get_if<int>(value))
    {
        return static_cast<float>(*intValue);
    }
    if (const auto *floatValue = std::get_if<float>(value))
    {
        return *floatValue;
    }
    if (const auto *stringValue = std::get_if<std::string>(value))
    {
        try
        {
            return std::stof(*stringValue);
        }
        catch (...)
        {
        }
    }

    return fallback;
}

static std::pair<float, float> parse_transform_translate(const std::string &transform)
{
    const auto open = transform.find('(');
    const auto close = transform.find(')', open == std::string::npos ? 0 : open);
    if (open == std::string::npos || close == std::string::npos || close <= open + 1)
    {
        return {0.0f, 0.0f};
    }

    std::string values = transform.substr(open + 1, close - open - 1);
    std::replace(values.begin(), values.end(), ',', ' ');

    std::istringstream stream(values);
    float x = 0.0f;
    float y = 0.0f;
    stream >> x;
    stream >> y;
    return {x, y};
}

static std::pair<float, float> element_transform_offset(Element *element)
{
    float x = get_element_float_attribute(element, "translateX", 0.0f);
    float y = get_element_float_attribute(element, "translateY", 0.0f);

    if (auto transform = get_element_string_attribute(element, "transform"))
    {
        const auto offset = parse_transform_translate(transform.value());
        x += offset.first;
        y += offset.second;
    }

    return {x, y};
}

static float element_absolute_left(Element *element)
{
    float result = 0.0f;
    for (Element *current = element; current != nullptr; current = current->getParent())
    {
        result += YGNodeLayoutGetLeft(current->getLayoutNode());
        result += element_transform_offset(current).first;
    }
    return result;
}

static float element_absolute_top(Element *element)
{
    float result = 0.0f;
    for (Element *current = element; current != nullptr; current = current->getParent())
    {
        result += YGNodeLayoutGetTop(current->getLayoutNode());
        result += element_transform_offset(current).second;
    }
    return result;
}

static bool element_ignores_pointer_events(Element *element)
{
    if (auto pointerEvents = get_element_string_attribute(element, "pointerEvents"))
    {
        std::string value = pointerEvents.value();
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch)
                       { return static_cast<char>(std::tolower(ch)); });
        return value == "none";
    }

    return false;
}

static bool is_element_hit(Element *element, float x, float y)
{
    if (element == nullptr || !element->getVisible() || element->getSceneGraph() == nullptr)
    {
        return false;
    }

    const float left = element_absolute_left(element);
    const float top = element_absolute_top(element);
    const float width = YGNodeLayoutGetWidth(element->getLayoutNode());
    const float height = YGNodeLayoutGetHeight(element->getLayoutNode());
    return width > 0.0f && height > 0.0f && x >= left && x <= left + width && y >= top && y <= top + height;
}

static Element *hit_test_element(Element *element, float x, float y)
{
    if (element == nullptr || !element->getVisible() || element->getSceneGraph() == nullptr)
    {
        return nullptr;
    }

    const auto &children = element->getChildren();
    for (auto it = children.rbegin(); it != children.rend(); ++it)
    {
        Element *hitChild = hit_test_element(*it, x, y);
        if (hitChild != nullptr)
        {
            return hitChild;
        }
    }

    if (element_ignores_pointer_events(element) || !is_element_hit(element, x, y))
    {
        return nullptr;
    }

    return element;
}

static uint64_t hit_test_event_target(JsEventRuntime *runtime, float x, float y)
{
    if (runtime == nullptr || runtime->scriptManager == nullptr || runtime->scriptManager->getSceneManager() == nullptr)
    {
        return 0;
    }

    SceneManager *sceneManager = runtime->scriptManager->getSceneManager();
    const std::vector<SceneGraph *> graphs = sceneManager->getSceneGraphs();
    for (auto it = graphs.rbegin(); it != graphs.rend(); ++it)
    {
        SceneGraph *graph = *it;
        if (graph == nullptr)
        {
            continue;
        }

        Element *target = hit_test_element(graph->getRoot(), x, y);
        if (target != nullptr)
        {
            return target->getUid();
        }
    }

    return 0;
}

static bool is_native_view_target(JsEventRuntime *runtime, uint64_t targetId)
{
    if (runtime == nullptr || runtime->scriptManager == nullptr || runtime->scriptManager->getSceneManager() == nullptr || targetId == 0)
    {
        return false;
    }

    return dynamic_cast<NativeViewElement *>(runtime->scriptManager->getSceneManager()->getElement(targetId)) != nullptr;
}

static JSValue create_base_event(JSContext *ctx, const char *type, uint64_t targetId)
{
    JSValue event = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, event, "type", JS_NewString(ctx, type));
    JS_SetPropertyStr(ctx, event, "target", JS_NewBigUint64(ctx, targetId));
    JS_SetPropertyStr(ctx, event, "currentTarget", JS_NewBigUint64(ctx, targetId));
    JS_SetPropertyStr(ctx, event, "bubbles", JS_NewBool(ctx, true));
    return event;
}

static int mouse_button_bit(MouseButton button)
{
    switch (button)
    {
    case MouseButton::Left:
        return 1;
    case MouseButton::Right:
        return 2;
    case MouseButton::Middle:
        return 4;
    default:
        return 0;
    }
}

static JSValue create_pointer_event(JSContext *ctx, const char *type, uint64_t targetId, const MouseEvent &event, float deltaX, float deltaY)
{
    JSValue jsEvent = create_base_event(ctx, type, targetId);
    const int button = static_cast<int>(event.button);
    JS_SetPropertyStr(ctx, jsEvent, "x", JS_NewFloat64(ctx, event.x));
    JS_SetPropertyStr(ctx, jsEvent, "y", JS_NewFloat64(ctx, event.y));
    JS_SetPropertyStr(ctx, jsEvent, "clientX", JS_NewFloat64(ctx, event.x));
    JS_SetPropertyStr(ctx, jsEvent, "clientY", JS_NewFloat64(ctx, event.y));
    JS_SetPropertyStr(ctx, jsEvent, "pageX", JS_NewFloat64(ctx, event.x));
    JS_SetPropertyStr(ctx, jsEvent, "pageY", JS_NewFloat64(ctx, event.y));
    JS_SetPropertyStr(ctx, jsEvent, "screenX", JS_NewFloat64(ctx, event.x));
    JS_SetPropertyStr(ctx, jsEvent, "screenY", JS_NewFloat64(ctx, event.y));
    JS_SetPropertyStr(ctx, jsEvent, "deltaX", JS_NewFloat64(ctx, deltaX));
    JS_SetPropertyStr(ctx, jsEvent, "deltaY", JS_NewFloat64(ctx, deltaY));
    JS_SetPropertyStr(ctx, jsEvent, "button", JS_NewInt32(ctx, button));
    JS_SetPropertyStr(ctx, jsEvent, "buttons", JS_NewInt32(ctx, event.buttons != 0 ? event.buttons : (event.isPressed ? mouse_button_bit(event.button) : 0)));
    JS_SetPropertyStr(ctx, jsEvent, "pointerId", JS_NewInt32(ctx, 1));
    JS_SetPropertyStr(ctx, jsEvent, "pointerType", JS_NewString(ctx, "mouse"));
    JS_SetPropertyStr(ctx, jsEvent, "isPrimary", JS_NewBool(ctx, true));
    return jsEvent;
}

static JSValue create_key_event(JSContext *ctx, const char *type, uint64_t targetId, const KeyEvent &event)
{
    JSValue jsEvent = create_base_event(ctx, type, targetId);
    JS_SetPropertyStr(ctx, jsEvent, "keyCode", JS_NewInt32(ctx, event.keyCode));
    JS_SetPropertyStr(ctx, jsEvent, "which", JS_NewInt32(ctx, event.keyCode));
    JS_SetPropertyStr(ctx, jsEvent, "altKey", JS_NewBool(ctx, event.altDown));
    JS_SetPropertyStr(ctx, jsEvent, "ctrlKey", JS_NewBool(ctx, event.controlDown));
    JS_SetPropertyStr(ctx, jsEvent, "shiftKey", JS_NewBool(ctx, event.shiftDown));
    JS_SetPropertyStr(ctx, jsEvent, "metaKey", JS_NewBool(ctx, event.metaDown));
    return jsEvent;
}

static JSValue create_wheel_event(JSContext *ctx, const MouseWheelEvent &event)
{
    JSValue jsEvent = create_base_event(ctx, "wheel", 0);
    JS_SetPropertyStr(ctx, jsEvent, "deltaY", JS_NewFloat64(ctx, -event.delta * 120.0f));
    JS_SetPropertyStr(ctx, jsEvent, "deltaMode", JS_NewInt32(ctx, 0));
    return jsEvent;
}

static JSValue create_window_event(JSContext *ctx, const char *type, const WindowEvent &event)
{
    JSValue jsEvent = create_base_event(ctx, type, 0);
    JS_SetPropertyStr(ctx, jsEvent, "width", JS_NewUint32(ctx, event.width));
    JS_SetPropertyStr(ctx, jsEvent, "height", JS_NewUint32(ctx, event.height));
    return jsEvent;
}

static void process_js_pending_jobs(JsEventRuntime *runtime)
{
    if (runtime == nullptr || runtime->runtime == nullptr)
    {
        return;
    }

    JSContext *pendingContext = nullptr;
    while (JS_ExecutePendingJob(runtime->runtime, &pendingContext) > 0)
    {
    }
}

static void call_event_handler(JsEventRuntime *runtime, JSValueConst callback, JSValue event)
{
    if (runtime == nullptr || runtime->context == nullptr)
    {
        return;
    }

    JSValue args[] = {event};
    JSValue result = JS_Call(runtime->context, callback, JS_UNDEFINED, 1, args);
    if (JS_IsException(result))
    {
        log_js_exception(runtime->context);
    }
    JS_FreeValue(runtime->context, result);
}

static bool js_event_stopped(JsEventRuntime *runtime, JSValue event)
{
    if (runtime == nullptr || runtime->context == nullptr)
    {
        return false;
    }

    JSValue stopped = JS_GetPropertyStr(runtime->context, event, "__stopped");
    const bool result = JS_ToBool(runtime->context, stopped) != 0;
    JS_FreeValue(runtime->context, stopped);
    return result;
}

static bool dispatch_event_to_path(JsEventRuntime *runtime, uint64_t targetId, const std::string &eventName, JSValue event)
{
    if (runtime == nullptr || runtime->scriptManager == nullptr || runtime->scriptManager->getSceneManager() == nullptr || targetId == 0)
    {
        return false;
    }

    Element *element = runtime->scriptManager->getSceneManager()->getElement(targetId);
    std::vector<std::pair<uint64_t, JSValue>> callbacks;
    for (Element *current = element; current != nullptr; current = current->getParent())
    {
        auto elementIt = runtime->handlers.find(current->getUid());
        if (elementIt == runtime->handlers.end())
        {
            continue;
        }

        auto handlerIt = elementIt->second.find(eventName);
        if (handlerIt == elementIt->second.end())
        {
            continue;
        }

        callbacks.push_back({current->getUid(), JS_DupValue(runtime->context, handlerIt->second)});
    }

    for (const auto &callback : callbacks)
    {
        JS_SetPropertyStr(runtime->context, event, "currentTarget", JS_NewBigUint64(runtime->context, callback.first));
        call_event_handler(runtime, callback.second, event);
        JS_FreeValue(runtime->context, callback.second);
        if (js_event_stopped(runtime, event))
        {
            break;
        }
    }

    process_js_pending_jobs(runtime);
    return !callbacks.empty();
}

static void dispatch_event_to_all(JsEventRuntime *runtime, const std::string &eventName, JSValue event)
{
    if (runtime == nullptr)
    {
        return;
    }

    std::vector<std::pair<uint64_t, JSValue>> callbacks;
    for (const auto &elementHandlers : runtime->handlers)
    {
        auto handlerIt = elementHandlers.second.find(eventName);
        if (handlerIt == elementHandlers.second.end())
        {
            continue;
        }

        callbacks.push_back({elementHandlers.first, JS_DupValue(runtime->context, handlerIt->second)});
    }

    for (const auto &callback : callbacks)
    {
        JS_SetPropertyStr(runtime->context, event, "target", JS_NewBigUint64(runtime->context, callback.first));
        JS_SetPropertyStr(runtime->context, event, "currentTarget", JS_NewBigUint64(runtime->context, callback.first));
        call_event_handler(runtime, callback.second, event);
        JS_FreeValue(runtime->context, callback.second);
        if (js_event_stopped(runtime, event))
        {
            break;
        }
    }

    process_js_pending_jobs(runtime);
}

static void dispatch_global_event(JsEventRuntime *runtime, const std::string &eventName, JSValue event)
{
    if (runtime == nullptr || runtime->context == nullptr)
    {
        return;
    }

    auto handlerIt = runtime->globalHandlers.find(eventName);
    if (handlerIt == runtime->globalHandlers.end())
    {
        return;
    }

    JSValue callback = JS_DupValue(runtime->context, handlerIt->second);
    call_event_handler(runtime, callback, event);
    JS_FreeValue(runtime->context, callback);
    process_js_pending_jobs(runtime);
}

static void dispatch_key_input(JsEventRuntime *runtime, const KeyEvent &event)
{
    if (runtime == nullptr || runtime->context == nullptr)
    {
        return;
    }

    if (runtime->focusedElementId == 0 && runtime->scriptManager != nullptr && runtime->scriptManager->getNativeViewRegistry() != nullptr &&
        runtime->scriptManager->getNativeViewRegistry()->dispatchKeyEvent(event))
    {
        return;
    }

    const std::string eventName = event.isPressed ? "onKeyDown" : "onKeyUp";
    const char *eventType = event.isPressed ? "keyDown" : "keyUp";
    const char *globalEventType = event.isPressed ? "keydown" : "keyup";
    const uint64_t targetId = runtime->focusedElementId;

    JSValue globalEvent = create_key_event(runtime->context, globalEventType, targetId, event);
    dispatch_global_event(runtime, globalEventType, globalEvent);
    JS_FreeValue(runtime->context, globalEvent);

    JSValue jsEvent = create_key_event(runtime->context, eventType, targetId, event);

    if (targetId == 0 || !dispatch_event_to_path(runtime, targetId, eventName, jsEvent))
    {
        dispatch_event_to_all(runtime, eventName, jsEvent);
    }

    JS_FreeValue(runtime->context, jsEvent);
}

static void dispatch_mouse_input(JsEventRuntime *runtime, const MouseEvent &event)
{
    if (runtime == nullptr || runtime->context == nullptr)
    {
        return;
    }

    if (event.type == MouseEvent::Type::Down)
    {
        const uint64_t targetId = hit_test_event_target(runtime, event.x, event.y);
        runtime->focusedElementId = targetId;
        runtime->pointerCaptureElementId = 0;
        runtime->pointerDownElementId = targetId;
        runtime->pointerDownX = event.x;
        runtime->pointerDownY = event.y;
        runtime->lastPointerX = event.x;
        runtime->lastPointerY = event.y;
        runtime->pointerDown = true;
        runtime->dragging = false;

        JSValue globalPointerEvent = create_pointer_event(runtime->context, "pointerdown", targetId, event, 0.0f, 0.0f);
        dispatch_global_event(runtime, "pointerdown", globalPointerEvent);
        JS_FreeValue(runtime->context, globalPointerEvent);

        JSValue globalMouseEvent = create_pointer_event(runtime->context, "mousedown", targetId, event, 0.0f, 0.0f);
        dispatch_global_event(runtime, "mousedown", globalMouseEvent);
        JS_FreeValue(runtime->context, globalMouseEvent);

        if (targetId != 0 && is_native_view_target(runtime, targetId) && runtime->scriptManager != nullptr && runtime->scriptManager->getNativeViewRegistry() != nullptr)
        {
            runtime->nativePointerCaptured = runtime->scriptManager->getNativeViewRegistry()->dispatchMouseEvent(event);
            if (runtime->nativePointerCaptured)
            {
                return;
            }
        }

        JSValue jsEvent = create_pointer_event(runtime->context, "pointerDown", targetId, event, 0.0f, 0.0f);
        dispatch_event_to_path(runtime, targetId, "onPointerDown", jsEvent);
        dispatch_event_to_path(runtime, targetId, "onMouseDown", jsEvent);
        JS_FreeValue(runtime->context, jsEvent);
        if (targetId == 0 && runtime->scriptManager != nullptr && runtime->scriptManager->getNativeViewRegistry() != nullptr)
        {
            runtime->nativePointerCaptured = runtime->scriptManager->getNativeViewRegistry()->dispatchMouseEvent(event);
        }
        return;
    }

    if (event.type == MouseEvent::Type::Move)
    {
        const bool routeToNative = runtime->nativePointerCaptured && runtime->scriptManager != nullptr && runtime->scriptManager->getNativeViewRegistry() != nullptr;
        const uint64_t targetId = routeToNative
                                      ? 0
                                      : (runtime->pointerCaptureElementId != 0
                                             ? runtime->pointerCaptureElementId
                                             : hit_test_event_target(runtime, event.x, event.y));
        const float deltaX = event.x - runtime->lastPointerX;
        const float deltaY = event.y - runtime->lastPointerY;

        JSValue globalPointerEvent = create_pointer_event(runtime->context, "pointermove", targetId, event, deltaX, deltaY);
        dispatch_global_event(runtime, "pointermove", globalPointerEvent);
        JS_FreeValue(runtime->context, globalPointerEvent);

        JSValue globalMouseEvent = create_pointer_event(runtime->context, "mousemove", targetId, event, deltaX, deltaY);
        dispatch_global_event(runtime, "mousemove", globalMouseEvent);
        JS_FreeValue(runtime->context, globalMouseEvent);

        JSValue jsEvent = create_pointer_event(runtime->context, "pointerMove", targetId, event, deltaX, deltaY);
        if (routeToNative)
        {
            runtime->scriptManager->getNativeViewRegistry()->dispatchMouseEvent(event);
            runtime->lastPointerX = event.x;
            runtime->lastPointerY = event.y;
            JS_FreeValue(runtime->context, jsEvent);
            return;
        }

        dispatch_event_to_path(runtime, targetId, "onPointerMove", jsEvent);
        dispatch_event_to_path(runtime, targetId, "onMouseMove", jsEvent);

        const uint64_t dragTargetId = runtime->pointerCaptureElementId != 0 ? runtime->pointerCaptureElementId : runtime->pointerDownElementId;
        if (runtime->pointerDown && dragTargetId != 0)
        {
            const float totalDeltaX = event.x - runtime->pointerDownX;
            const float totalDeltaY = event.y - runtime->pointerDownY;
            const float dragDistanceSquared = totalDeltaX * totalDeltaX + totalDeltaY * totalDeltaY;
            if (!runtime->dragging && dragDistanceSquared > 25.0f)
            {
                runtime->dragging = true;
                dispatch_event_to_path(runtime, dragTargetId, "onDragStart", jsEvent);
            }
            if (runtime->dragging)
            {
                dispatch_event_to_path(runtime, dragTargetId, "onDrag", jsEvent);
            }
        }

        runtime->lastPointerX = event.x;
        runtime->lastPointerY = event.y;
        JS_FreeValue(runtime->context, jsEvent);
        if (targetId == 0 && runtime->scriptManager != nullptr && runtime->scriptManager->getNativeViewRegistry() != nullptr)
        {
            runtime->scriptManager->getNativeViewRegistry()->dispatchMouseEvent(event);
        }
        return;
    }

    const bool routeToNative = runtime->nativePointerCaptured && runtime->scriptManager != nullptr && runtime->scriptManager->getNativeViewRegistry() != nullptr;
    const uint64_t targetId = routeToNative
                                  ? 0
                                  : (runtime->pointerCaptureElementId != 0
                                         ? runtime->pointerCaptureElementId
                                         : hit_test_event_target(runtime, event.x, event.y));
    const float deltaX = event.x - runtime->lastPointerX;
    const float deltaY = event.y - runtime->lastPointerY;

    JSValue globalPointerEvent = create_pointer_event(runtime->context, "pointerup", targetId, event, deltaX, deltaY);
    dispatch_global_event(runtime, "pointerup", globalPointerEvent);
    JS_FreeValue(runtime->context, globalPointerEvent);

    JSValue globalMouseEvent = create_pointer_event(runtime->context, "mouseup", targetId, event, deltaX, deltaY);
    dispatch_global_event(runtime, "mouseup", globalMouseEvent);
    JS_FreeValue(runtime->context, globalMouseEvent);

    JSValue jsEvent = create_pointer_event(runtime->context, "pointerUp", targetId, event, deltaX, deltaY);
    if (routeToNative)
    {
        runtime->scriptManager->getNativeViewRegistry()->dispatchMouseEvent(event);
        runtime->pointerDown = false;
        runtime->dragging = false;
        runtime->nativePointerCaptured = false;
        runtime->pointerCaptureElementId = 0;
        runtime->pointerDownElementId = 0;
        runtime->lastPointerX = event.x;
        runtime->lastPointerY = event.y;
        JS_FreeValue(runtime->context, jsEvent);
        return;
    }

    dispatch_event_to_path(runtime, targetId, "onPointerUp", jsEvent);
    dispatch_event_to_path(runtime, targetId, "onMouseUp", jsEvent);
    if (runtime->dragging)
    {
        const uint64_t dragTargetId = runtime->pointerCaptureElementId != 0 ? runtime->pointerCaptureElementId : runtime->pointerDownElementId;
        dispatch_event_to_path(runtime, dragTargetId, "onDragEnd", jsEvent);
    }
    else if (runtime->pointerDownElementId != 0 && runtime->pointerDownElementId == targetId)
    {
        JSValue clickEvent = create_pointer_event(runtime->context, "click", targetId, event, deltaX, deltaY);
        dispatch_event_to_path(runtime, targetId, "onClick", clickEvent);
        if (!js_event_stopped(runtime, clickEvent))
        {
            dispatch_global_event(runtime, "click", clickEvent);
        }
        JS_FreeValue(runtime->context, clickEvent);
    }

    runtime->pointerDown = false;
    runtime->dragging = false;
    runtime->nativePointerCaptured = false;
    runtime->pointerCaptureElementId = 0;
    runtime->pointerDownElementId = 0;
    runtime->lastPointerX = event.x;
    runtime->lastPointerY = event.y;
    JS_FreeValue(runtime->context, jsEvent);
    if (targetId == 0 && runtime->scriptManager != nullptr && runtime->scriptManager->getNativeViewRegistry() != nullptr)
    {
        runtime->scriptManager->getNativeViewRegistry()->dispatchMouseEvent(event);
    }
}

static void dispatch_window_input(JsEventRuntime *runtime, const WindowEvent &event)
{
    if (runtime == nullptr || runtime->context == nullptr)
    {
        return;
    }

    const char *eventType = "window";
    const char *handlerName = nullptr;
    const char *globalEventName = nullptr;
    switch (event.type)
    {
    case WindowEvent::Type::Resize:
        eventType = "windowResize";
        handlerName = "onWindowResize";
        globalEventName = "resize";
        break;
    case WindowEvent::Type::FocusGained:
        eventType = "windowFocus";
        handlerName = "onWindowFocus";
        globalEventName = "focus";
        break;
    case WindowEvent::Type::FocusLost:
        eventType = "windowBlur";
        handlerName = "onWindowBlur";
        globalEventName = "blur";
        break;
    case WindowEvent::Type::Close:
        eventType = "windowClose";
        handlerName = "onWindowClose";
        globalEventName = "close";
        break;
    }

    if (handlerName == nullptr)
    {
        return;
    }

    if (globalEventName != nullptr)
    {
        JSValue globalEvent = create_window_event(runtime->context, globalEventName, event);
        dispatch_global_event(runtime, globalEventName, globalEvent);
        JS_FreeValue(runtime->context, globalEvent);
    }

    JSValue jsEvent = create_window_event(runtime->context, eventType, event);
    dispatch_event_to_all(runtime, handlerName, jsEvent);
    JS_FreeValue(runtime->context, jsEvent);
}

static void handle_native_input_message(JSContext *ctx, const IMessage &message)
{
    JsEventRuntime *runtime = find_event_runtime(ctx);
    if (runtime == nullptr)
    {
        return;
    }

    if (const auto *keyEvent = std::get_if<KeyEvent>(&message.msg))
    {
        dispatch_key_input(runtime, *keyEvent);
    }
    else if (const auto *mouseEvent = std::get_if<MouseEvent>(&message.msg))
    {
        dispatch_mouse_input(runtime, *mouseEvent);
    }
    else if (const auto *wheelEvent = std::get_if<MouseWheelEvent>(&message.msg))
    {
        JSValue jsEvent = create_wheel_event(runtime->context, *wheelEvent);
        dispatch_global_event(runtime, "wheel", jsEvent);
        JS_FreeValue(runtime->context, jsEvent);
    }
}

static void handle_native_window_message(JSContext *ctx, const IMessage &message)
{
    JsEventRuntime *runtime = find_event_runtime(ctx);
    if (runtime == nullptr)
    {
        return;
    }

    if (const auto *windowEvent = std::get_if<WindowEvent>(&message.msg))
    {
        dispatch_window_input(runtime, *windowEvent);
    }
}

static void ensure_event_subscription(JsEventRuntime *runtime)
{
    if (runtime == nullptr || runtime->subscribed || runtime->scriptManager == nullptr)
    {
        return;
    }

    InputManager *inputManager = runtime->scriptManager->getInputManager();
    if (inputManager == nullptr)
    {
        return;
    }

    JSContext *ctx = runtime->context;
    inputManager->subscribeInput([ctx](const IMessage &message)
                                 { handle_native_input_message(ctx, message); });
    inputManager->subscribeWindowEvent([ctx](const IMessage &message)
                                       { handle_native_window_message(ctx, message); });
    runtime->subscribed = true;
}

static JSValue js_root_render(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv)
{
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (!sm || !sm->getSceneManager())
    {
        return JS_EXCEPTION;
    }

    uint64_t containerId = 0;
    if (JS_IsObject(this_val))
    {
        JSValue containerValue = JS_GetPropertyStr(ctx, this_val, "_containerId");
        js_read_element_id(ctx, containerValue, containerId);
        JS_FreeValue(ctx, containerValue);
    }

    uint64_t childId = 0;
    if (argc > 0)
    {
        js_read_element_id(ctx, argv[0], childId);
    }

    if (containerId == 0 || childId == 0)
    {
        return JS_ThrowInternalError(ctx, "Root render expects a valid container and child id");
    }

    sm->getSceneManager()->destroyAllChildren(containerId);
    sm->getSceneManager()->AppendElement(containerId, childId);
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

    if (argc < 2 || !js_read_element_id(ctx, argv[0], parentId) || !js_read_element_id(ctx, argv[1], childId) ||
        parentId == 0 || childId == 0)
    {
        return JS_ThrowInternalError(ctx, "appendChild expects valid parent and child ids");
    }

    sm->getSceneManager()->AppendElement(parentId, childId);

    return JS_UNDEFINED;
}

static JSValue js_insert_before(JSContext *ctx, JSValueConst this_val,
                                int argc, JSValueConst *argv)
{
    uint64_t parentId = 0, childId = 0, beforeChildId = 0;
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (!sm || !sm->getSceneManager())
    {
        return JS_EXCEPTION;
    }

    if (argc < 3 || !js_read_element_id(ctx, argv[0], parentId) || !js_read_element_id(ctx, argv[1], childId) ||
        !js_read_element_id(ctx, argv[2], beforeChildId) || parentId == 0 || childId == 0)
    {
        return JS_ThrowInternalError(ctx, "insertBefore expects valid parent, child, and beforeChild ids");
    }

    sm->getSceneManager()->InsertElementBefore(parentId, childId, beforeChildId);
    return JS_UNDEFINED;
}

static JSValue js_remove_child(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv)
{
    uint64_t parentId = 0, childId = 0;
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (!sm || !sm->getSceneManager())
    {
        return JS_EXCEPTION;
    }

    if (argc < 2 || !js_read_element_id(ctx, argv[0], parentId) || !js_read_element_id(ctx, argv[1], childId) ||
        parentId == 0 || childId == 0)
    {
        return JS_ThrowInternalError(ctx, "removeChild expects valid parent and child ids");
    }

    sm->getSceneManager()->RemoveElement(parentId, childId);
    return JS_UNDEFINED;
}

static JSValue js_clear_children(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv)
{
    uint64_t parentId = 0;
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (!sm || !sm->getSceneManager())
    {
        return JS_EXCEPTION;
    }

    if (argc < 1 || !js_read_element_id(ctx, argv[0], parentId) || parentId == 0)
    {
        return JS_ThrowInternalError(ctx, "clearChildren expects a valid parent id");
    }

    sm->getSceneManager()->destroyAllChildren(parentId);
    return JS_UNDEFINED;
}

// TODO REMOVE THIS FUNCTION and DEFINITION
static JSValue js_element_set_style(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv)
{

    return JS_UNDEFINED;
}

// helperFunction
Element::AttrValue ToElementAttributeValue(JSContext *context, JSValue value)
{
    if (JS_IsNull(value) || JS_IsUndefined(value))
    {
        return std::monostate();
    }
    else if (JS_IsBool(value))
    {
        return static_cast<bool>(JS_ToBool(context, value));
    }
    else if (JS_IsString(value))
    {
        const char *str = JS_ToCString(context, value);
        std::string res(str);
        JS_FreeCString(context, str);
        return res;
    }
    else if (JS_IsNumber(value))
    {
        double d;
        JS_ToFloat64(context, &d, value);
        if (d >= static_cast<double>(INT32_MIN) && d <= static_cast<double>(INT32_MAX))
        {
            return int(d);
        }
        return float(d);
    }

    return std::monostate();
}

/**
 *
 * arguments in argv
 * @param
 * node_ptr target node
 * prop_key : js object's property key(text, color and so on...)
 * prop_value : new value
 */
static JSValue js_update_props(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv)
{
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));

    if (!sm)
    {
        return JS_EXCEPTION;
    }

    SceneManager *sceneManager = sm->getSceneManager();
    if (!sceneManager)
    {
        return JS_EXCEPTION;
    }

    if (argc < 3)
    {
        //
        return JS_EXCEPTION;
    }
    uint64_t elementId = 0;
    if (!js_read_element_id(ctx, argv[0], elementId) || elementId == 0)
    {
        return JS_ThrowInternalError(ctx, "updateProps expects a valid element id");
    }

    const char *key = JS_ToCString(ctx, argv[1]);
    if (key == nullptr)
    {
        return JS_EXCEPTION;
    }

    auto value = ToElementAttributeValue(ctx, argv[2]);
    sceneManager->updateAttribute(elementId, key, value);

    JS_FreeCString(ctx, key);
    return JS_UNDEFINED;
}

static JSValue js_update_event_handler(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv)
{
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (sm == nullptr || argc < 3)
    {
        return JS_EXCEPTION;
    }

    uint64_t elementId = 0;
    if (!js_read_element_id(ctx, argv[0], elementId) || elementId == 0)
    {
        return JS_ThrowInternalError(ctx, "updateEventHandler expects a valid element id");
    }

    const char *key = JS_ToCString(ctx, argv[1]);
    if (key == nullptr)
    {
        return JS_EXCEPTION;
    }
    std::string eventName(key);
    JS_FreeCString(ctx, key);

    JsEventRuntime *runtime = ensure_event_runtime(ctx, sm);
    if (runtime == nullptr)
    {
        return JS_EXCEPTION;
    }

    auto &elementHandlers = runtime->handlers[elementId];
    auto handlerIt = elementHandlers.find(eventName);
    if (handlerIt != elementHandlers.end())
    {
        JS_FreeValue(ctx, handlerIt->second);
        elementHandlers.erase(handlerIt);
    }

    if (JS_IsFunction(ctx, argv[2]))
    {
        elementHandlers[eventName] = JS_DupValue(ctx, argv[2]);
        ensure_event_subscription(runtime);
    }

    if (elementHandlers.empty())
    {
        runtime->handlers.erase(elementId);
    }

    return JS_UNDEFINED;
}

static JSValue js_update_global_event_handler(JSContext *ctx, JSValueConst this_val,
                                              int argc, JSValueConst *argv)
{
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (sm == nullptr || argc < 2)
    {
        return JS_EXCEPTION;
    }

    const char *key = JS_ToCString(ctx, argv[0]);
    if (key == nullptr)
    {
        return JS_EXCEPTION;
    }
    std::string eventName(key);
    JS_FreeCString(ctx, key);

    JsEventRuntime *runtime = ensure_event_runtime(ctx, sm);
    if (runtime == nullptr)
    {
        return JS_EXCEPTION;
    }

    auto handlerIt = runtime->globalHandlers.find(eventName);
    if (handlerIt != runtime->globalHandlers.end())
    {
        JS_FreeValue(ctx, handlerIt->second);
        runtime->globalHandlers.erase(handlerIt);
    }

    if (argc >= 2 && JS_IsFunction(ctx, argv[1]))
    {
        runtime->globalHandlers[eventName] = JS_DupValue(ctx, argv[1]);
        ensure_event_subscription(runtime);
    }

    return JS_UNDEFINED;
}

static JSValue js_get_bounding_client_rect(JSContext *ctx, JSValueConst this_val,
                                           int argc, JSValueConst *argv)
{
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (sm == nullptr || sm->getSceneManager() == nullptr || argc < 1)
    {
        return JS_EXCEPTION;
    }

    uint64_t elementId = 0;
    if (!js_read_element_id(ctx, argv[0], elementId) || elementId == 0)
    {
        return JS_ThrowInternalError(ctx, "getBoundingClientRect expects a valid element id");
    }

    Element *element = sm->getSceneManager()->getElement(elementId);
    if (element == nullptr)
    {
        return JS_ThrowInternalError(ctx, "Element not found");
    }

    const float left = element_absolute_left(element);
    const float top = element_absolute_top(element);
    const float width = YGNodeLayoutGetWidth(element->getLayoutNode());
    const float height = YGNodeLayoutGetHeight(element->getLayoutNode());

    JSValue rect = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, rect, "x", JS_NewFloat64(ctx, left));
    JS_SetPropertyStr(ctx, rect, "y", JS_NewFloat64(ctx, top));
    JS_SetPropertyStr(ctx, rect, "left", JS_NewFloat64(ctx, left));
    JS_SetPropertyStr(ctx, rect, "top", JS_NewFloat64(ctx, top));
    JS_SetPropertyStr(ctx, rect, "right", JS_NewFloat64(ctx, left + width));
    JS_SetPropertyStr(ctx, rect, "bottom", JS_NewFloat64(ctx, top + height));
    JS_SetPropertyStr(ctx, rect, "width", JS_NewFloat64(ctx, width));
    JS_SetPropertyStr(ctx, rect, "height", JS_NewFloat64(ctx, height));
    return rect;
}
// TODO detachChild()

static JSValue js_create_text_node(JSContext *ctx, JSValueConst this_val,
                                   int argc, JSValueConst *argv)
{
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (!sm || !sm->getSceneManager())
    {
        return JS_EXCEPTION;
    }

    const char *text = nullptr;
    std::string textValue;
    if (argc > 0 && !JS_IsUndefined(argv[0]) && !JS_IsNull(argv[0]))
    {
        text = JS_ToCString(ctx, argv[0]);
        if (text == nullptr)
        {
            return JS_EXCEPTION;
        }
        textValue = text;
        JS_FreeCString(ctx, text);
    }

    Element *element = sm->getSceneManager()->createElement("text");
    if (element == nullptr)
    {
        return JS_ThrowInternalError(ctx, "Failed to create text node");
    }

    element->ApplyAttributes("text", textValue);
    JSValue result = JS_NewBigUint64(ctx, element->getUid());

    return result;
}

static JSValue js_update_text(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv)
{
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (!sm || !sm->getSceneManager())
    {
        return JS_EXCEPTION;
    }

    uint64_t elementId = 0;
    if (argc < 2 || !js_read_element_id(ctx, argv[0], elementId) || elementId == 0)
    {
        return JS_ThrowInternalError(ctx, "updateText expects a valid text node id and text");
    }

    const char *text = JS_ToCString(ctx, argv[1]);
    if (text == nullptr)
    {
        return JS_EXCEPTION;
    }

    sm->getSceneManager()->updateAttribute(elementId, "text", std::string(text));
    JS_FreeCString(ctx, text);
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

struct NativeFetchRequest
{
    std::string url;
    std::string method = "GET";
    std::string headers;
    std::string body;
    bool hasBody = false;
};

struct NativeFetchResponse
{
    int status = 0;
    std::string statusText;
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;
    std::string error;
};

static std::string to_upper_ascii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

static bool js_value_to_std_string(JSContext *ctx, JSValueConst value, std::string &out)
{
    size_t length = 0;
    const char *data = JS_ToCStringLen(ctx, &length, value);
    if (data == nullptr)
    {
        return false;
    }

    out.assign(data, length);
    JS_FreeCString(ctx, data);
    return true;
}

static bool js_get_optional_string_property(JSContext *ctx, JSValueConst object, const char *name, std::string &out)
{
    JSValue value = JS_GetPropertyStr(ctx, object, name);
    if (JS_IsException(value))
    {
        return false;
    }

    if (JS_IsUndefined(value) || JS_IsNull(value))
    {
        JS_FreeValue(ctx, value);
        return true;
    }

    bool ok = js_value_to_std_string(ctx, value, out);
    JS_FreeValue(ctx, value);
    return ok;
}

static bool has_cr_or_lf(const std::string &value)
{
    return value.find_first_of("\r\n") != std::string::npos;
}

static void append_fetch_header_line(const std::string &name, const std::string &value, std::string &headersOut)
{
    if (name.empty() || name.find(':') != std::string::npos || has_cr_or_lf(name) || has_cr_or_lf(value))
    {
        return;
    }

    headersOut += name;
    headersOut += ": ";
    headersOut += value;
    headersOut += "\r\n";
}

static bool append_js_fetch_headers(JSContext *ctx, JSValueConst headersValue, std::string &headersOut)
{
    if (JS_IsUndefined(headersValue) || JS_IsNull(headersValue))
    {
        return true;
    }

    if (JS_IsString(headersValue))
    {
        std::string rawHeaders;
        if (!js_value_to_std_string(ctx, headersValue, rawHeaders))
        {
            return false;
        }

        headersOut += rawHeaders;
        if (headersOut.size() < 2 || headersOut.substr(headersOut.size() - 2) != "\r\n")
        {
            headersOut += "\r\n";
        }
        return true;
    }

    if (!JS_IsObject(headersValue))
    {
        return true;
    }

    JSPropertyEnum *properties = nullptr;
    uint32_t propertyCount = 0;
    if (JS_GetOwnPropertyNames(ctx, &properties, &propertyCount, headersValue, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0)
    {
        return false;
    }

    bool ok = true;
    for (uint32_t index = 0; index < propertyCount; ++index)
    {
        const char *name = JS_AtomToCString(ctx, properties[index].atom);
        if (name == nullptr)
        {
            ok = false;
            continue;
        }

        JSValue value = JS_GetProperty(ctx, headersValue, properties[index].atom);
        if (JS_IsException(value))
        {
            JS_FreeCString(ctx, name);
            ok = false;
            continue;
        }

        if (!JS_IsUndefined(value) && !JS_IsNull(value))
        {
            std::string headerValue;
            if (js_value_to_std_string(ctx, value, headerValue))
            {
                append_fetch_header_line(name, headerValue, headersOut);
            }
            else
            {
                ok = false;
            }
        }

        JS_FreeValue(ctx, value);
        JS_FreeCString(ctx, name);
    }

    JS_FreePropertyEnum(ctx, properties, propertyCount);
    return ok;
}

static bool read_fetch_request(JSContext *ctx, int argc, JSValueConst *argv, NativeFetchRequest &request)
{
    if (argc < 1 || JS_IsUndefined(argv[0]) || JS_IsNull(argv[0]))
    {
        JS_ThrowTypeError(ctx, "fetchSync expects a URL");
        return false;
    }

    if (!js_value_to_std_string(ctx, argv[0], request.url))
    {
        return false;
    }

    if (argc < 2 || !JS_IsObject(argv[1]))
    {
        return true;
    }

    std::string method;
    if (!js_get_optional_string_property(ctx, argv[1], "method", method))
    {
        return false;
    }
    if (!method.empty())
    {
        request.method = to_upper_ascii(method);
    }

    JSValue headersValue = JS_GetPropertyStr(ctx, argv[1], "headers");
    if (JS_IsException(headersValue))
    {
        return false;
    }
    bool headersOk = append_js_fetch_headers(ctx, headersValue, request.headers);
    JS_FreeValue(ctx, headersValue);
    if (!headersOk)
    {
        return false;
    }

    JSValue bodyValue = JS_GetPropertyStr(ctx, argv[1], "body");
    if (JS_IsException(bodyValue))
    {
        return false;
    }
    if (!JS_IsUndefined(bodyValue) && !JS_IsNull(bodyValue))
    {
        request.hasBody = true;
        if (!js_value_to_std_string(ctx, bodyValue, request.body))
        {
            JS_FreeValue(ctx, bodyValue);
            return false;
        }
    }
    JS_FreeValue(ctx, bodyValue);

    return true;
}

#ifdef _WIN32
struct WinHttpScopedHandle
{
    HINTERNET value = nullptr;

    ~WinHttpScopedHandle()
    {
        if (value != nullptr)
        {
            WinHttpCloseHandle(value);
        }
    }

    operator HINTERNET() const
    {
        return value;
    }
};

static std::wstring utf8_to_wide(const std::string &value)
{
    if (value.empty())
    {
        return {};
    }

    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (length <= 0)
    {
        length = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    }
    if (length <= 0)
    {
        return {};
    }

    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), &result[0], length);
    return result;
}

static std::string wide_to_utf8(const std::wstring &value)
{
    if (value.empty())
    {
        return {};
    }

    int length = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0)
    {
        return {};
    }

    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), &result[0], length, nullptr, nullptr);
    return result;
}

static std::wstring trim_wide(std::wstring value)
{
    while (!value.empty() && std::iswspace(value.front()))
    {
        value.erase(value.begin());
    }
    while (!value.empty() && std::iswspace(value.back()))
    {
        value.pop_back();
    }
    return value;
}

static std::string winhttp_error(const char *operation)
{
    return std::string(operation) + " failed with Windows error " + std::to_string(GetLastError());
}

static void collect_response_headers(const std::wstring &rawHeaders, NativeFetchResponse &response)
{
    size_t start = 0;
    bool firstLine = true;
    while (start <= rawHeaders.size())
    {
        size_t end = rawHeaders.find(L"\r\n", start);
        std::wstring line = end == std::wstring::npos ? rawHeaders.substr(start) : rawHeaders.substr(start, end - start);
        if (line.empty())
        {
            break;
        }

        if (!firstLine)
        {
            size_t colon = line.find(L':');
            if (colon != std::wstring::npos)
            {
                std::wstring name = trim_wide(line.substr(0, colon));
                std::wstring value = trim_wide(line.substr(colon + 1));
                if (!name.empty())
                {
                    response.headers.emplace_back(wide_to_utf8(name), wide_to_utf8(value));
                }
            }
        }

        firstLine = false;
        if (end == std::wstring::npos)
        {
            break;
        }
        start = end + 2;
    }
}

static NativeFetchResponse perform_native_fetch(const NativeFetchRequest &request)
{
    NativeFetchResponse response;
    std::wstring wideUrl = utf8_to_wide(request.url);
    if (wideUrl.empty())
    {
        response.error = "fetch URL must be a valid UTF-8 string";
        return response;
    }

    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    components.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(wideUrl.c_str(), static_cast<DWORD>(wideUrl.size()), 0, &components))
    {
        response.error = winhttp_error("WinHttpCrackUrl");
        return response;
    }

    if (components.nScheme != INTERNET_SCHEME_HTTP && components.nScheme != INTERNET_SCHEME_HTTPS)
    {
        response.error = "fetch only supports http:// and https:// URLs";
        return response;
    }

    std::wstring host;
    if (components.lpszHostName != nullptr && components.dwHostNameLength > 0)
    {
        host.assign(components.lpszHostName, components.dwHostNameLength);
    }
    if (host.empty())
    {
        response.error = "fetch URL must include a host";
        return response;
    }

    std::wstring path = L"/";
    if (components.lpszUrlPath != nullptr && components.dwUrlPathLength > 0)
    {
        path.assign(components.lpszUrlPath, components.dwUrlPathLength);
    }
    if (components.lpszExtraInfo != nullptr && components.dwExtraInfoLength > 0)
    {
        path.append(components.lpszExtraInfo, components.dwExtraInfoLength);
    }

    if (request.body.size() > MAXDWORD || request.headers.size() > MAXDWORD)
    {
        response.error = "fetch request is too large";
        return response;
    }

    WinHttpScopedHandle session{WinHttpOpen(L"MachiUI/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
    if (!session)
    {
        response.error = winhttp_error("WinHttpOpen");
        return response;
    }
    WinHttpSetTimeouts(session, 30000, 30000, 30000, 30000);

    WinHttpScopedHandle connect{WinHttpConnect(session, host.c_str(), components.nPort, 0)};
    if (!connect)
    {
        response.error = winhttp_error("WinHttpConnect");
        return response;
    }

    std::wstring method = utf8_to_wide(request.method.empty() ? std::string("GET") : request.method);
    DWORD flags = components.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    WinHttpScopedHandle httpRequest{WinHttpOpenRequest(connect, method.c_str(), path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags)};
    if (!httpRequest)
    {
        response.error = winhttp_error("WinHttpOpenRequest");
        return response;
    }

    std::wstring wideHeaders = utf8_to_wide(request.headers);
    LPCWSTR headerPtr = wideHeaders.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : wideHeaders.c_str();
    DWORD headerLength = wideHeaders.empty() ? 0 : static_cast<DWORD>(wideHeaders.size());
    DWORD bodyLength = request.hasBody ? static_cast<DWORD>(request.body.size()) : 0;
    LPVOID bodyPtr = bodyLength == 0 ? WINHTTP_NO_REQUEST_DATA : const_cast<char *>(request.body.data());

    if (!WinHttpSendRequest(httpRequest, headerPtr, headerLength, bodyPtr, bodyLength, bodyLength, 0))
    {
        response.error = winhttp_error("WinHttpSendRequest");
        return response;
    }

    if (!WinHttpReceiveResponse(httpRequest, nullptr))
    {
        response.error = winhttp_error("WinHttpReceiveResponse");
        return response;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (WinHttpQueryHeaders(httpRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX))
    {
        response.status = static_cast<int>(statusCode);
    }

    DWORD statusTextSize = 0;
    if (!WinHttpQueryHeaders(httpRequest, WINHTTP_QUERY_STATUS_TEXT, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER, &statusTextSize, WINHTTP_NO_HEADER_INDEX) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER && statusTextSize > 0)
    {
        std::wstring statusText(statusTextSize / sizeof(wchar_t), L'\0');
        if (WinHttpQueryHeaders(httpRequest, WINHTTP_QUERY_STATUS_TEXT, WINHTTP_HEADER_NAME_BY_INDEX, &statusText[0], &statusTextSize, WINHTTP_NO_HEADER_INDEX))
        {
            size_t charCount = statusTextSize / sizeof(wchar_t);
            if (charCount > 0 && statusText[charCount - 1] == L'\0')
            {
                --charCount;
            }
            statusText.resize(charCount);
            response.statusText = wide_to_utf8(statusText);
        }
    }

    DWORD rawHeaderSize = 0;
    if (!WinHttpQueryHeaders(httpRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER, &rawHeaderSize, WINHTTP_NO_HEADER_INDEX) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER && rawHeaderSize > 0)
    {
        std::wstring rawHeaders(rawHeaderSize / sizeof(wchar_t), L'\0');
        if (WinHttpQueryHeaders(httpRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, &rawHeaders[0], &rawHeaderSize, WINHTTP_NO_HEADER_INDEX))
        {
            size_t charCount = rawHeaderSize / sizeof(wchar_t);
            if (charCount > 0 && rawHeaders[charCount - 1] == L'\0')
            {
                --charCount;
            }
            rawHeaders.resize(charCount);
            collect_response_headers(rawHeaders, response);
        }
    }

    DWORD available = 0;
    do
    {
        available = 0;
        if (!WinHttpQueryDataAvailable(httpRequest, &available))
        {
            response.error = winhttp_error("WinHttpQueryDataAvailable");
            return response;
        }

        if (available == 0)
        {
            break;
        }

        size_t offset = response.body.size();
        response.body.resize(offset + available);
        DWORD downloaded = 0;
        if (!WinHttpReadData(httpRequest, &response.body[offset], available, &downloaded))
        {
            response.error = winhttp_error("WinHttpReadData");
            return response;
        }
        response.body.resize(offset + downloaded);
    } while (available > 0);

    return response;
}
#else
static NativeFetchResponse perform_native_fetch(const NativeFetchRequest &)
{
    NativeFetchResponse response;
    response.error = "fetch is not implemented on this platform";
    return response;
}
#endif

static JSValue js_fetch_response_to_value(JSContext *ctx, const NativeFetchRequest &request, const NativeFetchResponse &response)
{
    JSValue result = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, result, "ok", JS_NewBool(ctx, response.status >= 200 && response.status < 300));
    JS_SetPropertyStr(ctx, result, "status", JS_NewInt32(ctx, response.status));
    JS_SetPropertyStr(ctx, result, "statusText", JS_NewString(ctx, response.statusText.c_str()));
    JS_SetPropertyStr(ctx, result, "url", JS_NewString(ctx, request.url.c_str()));
    JS_SetPropertyStr(ctx, result, "body", JS_NewStringLen(ctx, response.body.data(), response.body.size()));

    JSValue headers = JS_NewArray(ctx);
    for (uint32_t index = 0; index < response.headers.size(); ++index)
    {
        JSValue entry = JS_NewArray(ctx);
        JS_SetPropertyUint32(ctx, entry, 0, JS_NewString(ctx, response.headers[index].first.c_str()));
        JS_SetPropertyUint32(ctx, entry, 1, JS_NewString(ctx, response.headers[index].second.c_str()));
        JS_SetPropertyUint32(ctx, headers, index, entry);
    }
    JS_SetPropertyStr(ctx, result, "headers", headers);

    return result;
}

static JSValue js_is_network_enabled(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv)
{
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    return JS_NewBool(ctx, sm != nullptr && sm->isNetworkEnabled());
}

static JSValue js_fetch_sync(JSContext *ctx, JSValueConst this_val,
                             int argc, JSValueConst *argv)
{
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (sm == nullptr || !sm->isNetworkEnabled())
    {
        return JS_ThrowTypeError(ctx, "MachiUI network capability is disabled. Enable ScriptManager::setNetworkEnabled(true) from the host runtime before using fetch().");
    }

    NativeFetchRequest request;
    if (!read_fetch_request(ctx, argc, argv, request))
    {
        return JS_EXCEPTION;
    }

    NativeFetchResponse response = perform_native_fetch(request);
    if (!response.error.empty())
    {
        return JS_ThrowInternalError(ctx, "%s", response.error.c_str());
    }

    return js_fetch_response_to_value(ctx, request, response);
}

static JSValue js_action_result_to_value(JSContext *ctx, const ActionResult &result)
{
    JSValue value = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, value, "ok", JS_NewBool(ctx, result.ok));
    JS_SetPropertyStr(ctx, value, "payload", JS_NewStringLen(ctx, result.payloadJson.data(), result.payloadJson.size()));
    JS_SetPropertyStr(ctx, value, "error", JS_NewString(ctx, result.error.c_str()));
    return value;
}

static JSValue js_invoke_action(JSContext *ctx, JSValueConst this_val,
                                int argc, JSValueConst *argv)
{
    ScriptManager *sm = static_cast<ScriptManager *>(JS_GetContextOpaque(ctx));
    if (sm == nullptr || sm->getActionRegistry() == nullptr)
    {
        return JS_ThrowTypeError(ctx, "MachiUI action registry is not available");
    }

    if (argc < 1 || JS_IsUndefined(argv[0]) || JS_IsNull(argv[0]))
    {
        return JS_ThrowTypeError(ctx, "invokeAction expects an action name");
    }

    std::string name;
    if (!js_value_to_std_string(ctx, argv[0], name))
    {
        return JS_EXCEPTION;
    }

    std::string payloadJson = "null";
    if (argc > 1 && !JS_IsUndefined(argv[1]) && !JS_IsNull(argv[1]))
    {
        if (!js_value_to_std_string(ctx, argv[1], payloadJson))
        {
            return JS_EXCEPTION;
        }
    }

    ActionResult result = sm->getActionRegistry()->invoke(name, payloadJson);
    return js_action_result_to_value(ctx, result);
}

static void install_fetch_shim(JSContext *ctx)
{
    static const char *source = R"JS(
(function () {
  function isPresent(value) {
    return value !== undefined && value !== null;
  }

  function Headers(init) {
    this._map = {};
    if (init) {
      this._init(init);
    }
  }

  Headers.prototype._normalizeName = function (name) {
    return String(name).toLowerCase();
  };

  Headers.prototype._init = function (init) {
    var index;
    var entry;

    if (init instanceof Headers) {
      var copied = init.toArray();
      for (index = 0; index < copied.length; index++) {
        this.append(copied[index][0], copied[index][1]);
      }
      return;
    }

    if (Array.isArray(init)) {
      for (index = 0; index < init.length; index++) {
        entry = init[index];
        if (entry && entry.length >= 2) {
          this.append(entry[0], entry[1]);
        }
      }
      return;
    }

    if (typeof init === "object") {
      for (var key in init) {
        if (Object.prototype.hasOwnProperty.call(init, key)) {
          this.set(key, init[key]);
        }
      }
    }
  };

  Headers.prototype.append = function (name, value) {
    var key = this._normalizeName(name);
    var stringValue = String(value);
    if (this._map[key]) {
      this._map[key].value += ", " + stringValue;
    } else {
      this._map[key] = { name: String(name), value: stringValue };
    }
  };

  Headers.prototype.set = function (name, value) {
    this._map[this._normalizeName(name)] = { name: String(name), value: String(value) };
  };

  Headers.prototype.get = function (name) {
    var entry = this._map[this._normalizeName(name)];
    return entry ? entry.value : null;
  };

  Headers.prototype.has = function (name) {
    return this._map[this._normalizeName(name)] !== undefined;
  };

  Headers.prototype.delete = function (name) {
    delete this._map[this._normalizeName(name)];
  };

  Headers.prototype.forEach = function (callback, thisArg) {
    var entries = this.toArray();
    for (var index = 0; index < entries.length; index++) {
      callback.call(thisArg, entries[index][1], entries[index][0], this);
    }
  };

  Headers.prototype.toArray = function () {
    var result = [];
    for (var key in this._map) {
      if (Object.prototype.hasOwnProperty.call(this._map, key)) {
        result.push([this._map[key].name, this._map[key].value]);
      }
    }
    return result;
  };

  Headers.prototype.entries = function () {
    var entries = this.toArray();
    return entries[Symbol.iterator]();
  };

  Headers.prototype.keys = function () {
    return this.toArray().map(function (entry) { return entry[0]; })[Symbol.iterator]();
  };

  Headers.prototype.values = function () {
    return this.toArray().map(function (entry) { return entry[1]; })[Symbol.iterator]();
  };

  if (typeof Symbol !== "undefined" && Symbol.iterator) {
    Headers.prototype[Symbol.iterator] = Headers.prototype.entries;
  }

  function Response(body, init) {
    init = init || {};
    this._body = isPresent(body) ? String(body) : "";
    this.status = init.status === undefined ? 200 : Number(init.status);
    this.statusText = init.statusText === undefined ? "" : String(init.statusText);
    this.headers = new Headers(init.headers);
    this.ok = this.status >= 200 && this.status < 300;
    this.url = init.url === undefined ? "" : String(init.url);
    this.redirected = false;
    this.type = "basic";
    this.bodyUsed = false;
  }

  Response.prototype.text = function () {
    this.bodyUsed = true;
    return Promise.resolve(this._body);
  };

  Response.prototype.json = function () {
    return this.text().then(function (text) {
      return JSON.parse(text);
    });
  };

  Response.prototype.arrayBuffer = function () {
    this.bodyUsed = true;
    var buffer = new ArrayBuffer(this._body.length);
    var view = new Uint8Array(buffer);
    for (var index = 0; index < this._body.length; index++) {
      view[index] = this._body.charCodeAt(index) & 255;
    }
    return Promise.resolve(buffer);
  };

  Response.prototype.clone = function () {
    return new Response(this._body, {
      status: this.status,
      statusText: this.statusText,
      headers: this.headers,
      url: this.url
    });
  };

  function Request(input, init) {
    init = init || {};

    if (input instanceof Request) {
      this.url = input.url;
      this.method = input.method;
      this.headers = new Headers(input.headers);
      this.body = input.body;
    } else if (input && typeof input === "object" && input.url !== undefined) {
      this.url = String(input.url);
      this.method = input.method ? String(input.method) : "GET";
      this.headers = new Headers(input.headers);
      this.body = input.body;
    } else {
      this.url = String(input);
      this.method = "GET";
      this.headers = new Headers();
      this.body = undefined;
    }

    if (init.method !== undefined) {
      this.method = String(init.method);
    }
    this.method = this.method.toUpperCase();

    if (init.headers !== undefined) {
      this.headers = new Headers(init.headers);
    }
    if (init.body !== undefined) {
      this.body = init.body;
    }
  }

  Request.prototype.clone = function () {
    return new Request(this);
  };

  if (typeof globalThis.Headers === "undefined") {
    globalThis.Headers = Headers;
  }
  if (typeof globalThis.Response === "undefined") {
    globalThis.Response = Response;
  }
  if (typeof globalThis.Request === "undefined") {
    globalThis.Request = Request;
  }

  globalThis.fetch = function (input, init) {
    return new Promise(function (resolve, reject) {
      try {
        var request = new Request(input, init || {});
        var headers = {};
        request.headers.forEach(function (value, name) {
          headers[name] = value;
        });

        var nativeResponse = MachiNative.fetchSync(request.url, {
          method: request.method,
          headers: headers,
          body: request.body
        });

        resolve(new Response(nativeResponse.body, {
          status: nativeResponse.status,
          statusText: nativeResponse.statusText,
          headers: nativeResponse.headers,
          url: nativeResponse.url
        }));
      } catch (error) {
        reject(error);
      }
    });
  };

  var runtime = globalThis.MachiRuntime || {};
  runtime.hasCapability = function (name) {
    return name === "network" ? MachiNative.isNetworkEnabled() : false;
  };
  try {
    Object.defineProperty(runtime, "capabilities", {
      configurable: true,
      enumerable: true,
      get: function () {
        return { network: MachiNative.isNetworkEnabled() };
      }
    });
  } catch (error) {
    runtime.capabilities = { network: MachiNative.isNetworkEnabled() };
  }
  globalThis.MachiRuntime = runtime;

  function parseActionPayload(payload) {
    if (payload === undefined || payload === null || payload === "") {
      return null;
    }
    try {
      return JSON.parse(payload);
    } catch (error) {
      return payload;
    }
  }

  function stringifyActionPayload(payload) {
    if (payload === undefined) {
      return "null";
    }
    return JSON.stringify(payload);
  }

  var machi = globalThis.Machi || {};
  var actions = machi.actions || {};
  actions.invokeSync = function (name, payload) {
    var result = MachiNative.invokeAction(String(name), stringifyActionPayload(payload));
    if (!result.ok) {
      throw new Error(result.error || ("Action failed: " + name));
    }
    return parseActionPayload(result.payload);
  };
  actions.invoke = function (name, payload) {
    return new Promise(function (resolve, reject) {
      try {
        resolve(actions.invokeSync(name, payload));
      } catch (error) {
        reject(error);
      }
    });
  };
  machi.actions = actions;
  globalThis.Machi = machi;
})();
)JS";

    JSValue result = JS_Eval(ctx, source, std::strlen(source), "<machi-fetch-shim>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result))
    {
        log_js_exception(ctx);
    }
    JS_FreeValue(ctx, result);
}

// Binding Default Elements (Root, Text, Image, etc.)
void register_default_elements(JSContext *ctx)
{
    // GenericBinder({
    //                   "Element",
    //                   element_funcs,
    //                   sizeof(element_funcs) / sizeof(JSCFunctionListEntry),
    //                   nullptr // 부모 클래스 없음
    //               })
    //     .Bind(ctx);

    // GenericBinder({
    //                   "Root",
    //                   root_funcs,
    //                   sizeof(root_funcs) / sizeof(JSCFunctionListEntry),
    //                   "Element" // Root는 Element를 상속
    //               })
    //     .Bind(ctx);
}

// 네이티브 메서드 등록 함수
void register_native_method(JSContext *ctx, ScriptManager *manager)
{
    ensure_event_runtime(ctx, manager);

    JSValue native = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, native, "createElement", JS_NewCFunction(ctx, js_create_element, "createElement", 1));
    JS_SetPropertyStr(ctx, native, "createRoot", JS_NewCFunction(ctx, js_create_root, "createRoot", 1));
    JS_SetPropertyStr(ctx, native, "appendChild", JS_NewCFunction(ctx, js_append_child, "appendChild", 2));
    JS_SetPropertyStr(ctx, native, "insertBefore", JS_NewCFunction(ctx, js_insert_before, "insertBefore", 3));
    JS_SetPropertyStr(ctx, native, "removeChild", JS_NewCFunction(ctx, js_remove_child, "removeChild", 2));
    JS_SetPropertyStr(ctx, native, "clearChildren", JS_NewCFunction(ctx, js_clear_children, "clearChildren", 1));
    JS_SetPropertyStr(ctx, native, "updateProps", JS_NewCFunction(ctx, js_update_props, "updateProps", 3));
    JS_SetPropertyStr(ctx, native, "updateEventHandler", JS_NewCFunction(ctx, js_update_event_handler, "updateEventHandler", 3));
    JS_SetPropertyStr(ctx, native, "updateGlobalEventHandler", JS_NewCFunction(ctx, js_update_global_event_handler, "updateGlobalEventHandler", 2));
    JS_SetPropertyStr(ctx, native, "getBoundingClientRect", JS_NewCFunction(ctx, js_get_bounding_client_rect, "getBoundingClientRect", 1));
    JS_SetPropertyStr(ctx, native, "createTextNode", JS_NewCFunction(ctx, js_create_text_node, "createTextNode", 1));
    JS_SetPropertyStr(ctx, native, "updateText", JS_NewCFunction(ctx, js_update_text, "updateText", 2));
    JS_SetPropertyStr(ctx, native, "isNetworkEnabled", JS_NewCFunction(ctx, js_is_network_enabled, "isNetworkEnabled", 0));
    JS_SetPropertyStr(ctx, native, "fetchSync", JS_NewCFunction(ctx, js_fetch_sync, "fetchSync", 2));
    JS_SetPropertyStr(ctx, native, "invokeAction", JS_NewCFunction(ctx, js_invoke_action, "invokeAction", 2));

    register_default_elements(ctx);
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "MachiNative", native);
    JS_FreeValue(ctx, global);
    install_fetch_shim(ctx);
}
