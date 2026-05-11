#pragma once

#include "IService.h"
#include "Types.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct NativeViewSlot
{
    uint64_t elementId = 0;
    std::string nativeType;
    std::string viewId;
    Rect rect{0.0f, 0.0f, 0.0f, 0.0f};
    bool visible = true;
    int zIndex = 0;
};

struct NativeViewInputEvent
{
    enum class Type
    {
        PointerMove,
        PointerDown,
        PointerUp,
        MouseWheel,
        KeyDown,
        KeyUp
    } type = Type::PointerMove;

    uint64_t elementId = 0;
    std::string viewId;
    float x = 0.0f;
    float y = 0.0f;
    float localX = 0.0f;
    float localY = 0.0f;
    float delta = 0.0f;
    int button = 0;
    int keyCode = 0;
    bool altDown = false;
    bool controlDown = false;
    bool shiftDown = false;
    bool metaDown = false;
};

class INativeViewAdapter
{
public:
    virtual ~INativeViewAdapter() = default;

    virtual void mount(const NativeViewSlot &slot) {}
    virtual void update(const NativeViewSlot &slot) {}
    virtual void unmount() {}
    virtual bool dispatchInput(const NativeViewInputEvent &event) { return false; }
};

class NativeViewRegistry : public IService
{
public:
    using NativeViewFactory = std::function<std::unique_ptr<INativeViewAdapter>(const NativeViewSlot &)>;

    void onInit(ServiceProvider *provider) override;

    void registerFactory(const std::string &nativeType, NativeViewFactory factory);
    void unregisterFactory(const std::string &nativeType);

    void beginSync();
    void syncSlot(const NativeViewSlot &slot);
    void endSync();

    const std::vector<NativeViewSlot> &getSlots() const;

    bool dispatchMouseEvent(const MouseEvent &event);
    bool dispatchMouseWheelEvent(const MouseWheelEvent &event, float x, float y);
    bool dispatchKeyEvent(const KeyEvent &event);

private:
    struct ActiveNativeView
    {
        NativeViewSlot slot;
        std::unique_ptr<INativeViewAdapter> adapter;
        bool seenThisSync = false;
    };

    std::unordered_map<std::string, NativeViewFactory> factories;
    std::unordered_map<uint64_t, ActiveNativeView> activeViews;
    std::vector<NativeViewSlot> slots;
    uint64_t capturedElementId = 0;
    uint64_t focusedElementId = 0;

    uint64_t hitTest(float x, float y) const;
    bool dispatchTo(uint64_t elementId, NativeViewInputEvent event);
    void ensureAdapter(ActiveNativeView &view);
};
