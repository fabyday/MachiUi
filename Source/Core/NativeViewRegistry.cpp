#include "NativeViewRegistry.h"
#include "ServiceRegistry.h"

#include <algorithm>
#include <utility>

void NativeViewRegistry::onInit(ServiceProvider *provider)
{
}

void NativeViewRegistry::registerFactory(const std::string &nativeType, NativeViewFactory factory)
{
    if (nativeType.empty() || !factory)
    {
        return;
    }

    factories[nativeType] = std::move(factory);
}

void NativeViewRegistry::unregisterFactory(const std::string &nativeType)
{
    factories.erase(nativeType);
}

void NativeViewRegistry::beginSync()
{
    slots.clear();
    for (auto &entry : activeViews)
    {
        entry.second.seenThisSync = false;
    }
}

void NativeViewRegistry::ensureAdapter(ActiveNativeView &view)
{
    if (view.adapter != nullptr)
    {
        return;
    }

    auto factoryIt = factories.find(view.slot.nativeType);
    if (factoryIt == factories.end())
    {
        return;
    }

    view.adapter = factoryIt->second(view.slot);
    if (view.adapter != nullptr)
    {
        view.adapter->mount(view.slot);
    }
}

void NativeViewRegistry::syncSlot(const NativeViewSlot &slot)
{
    if (slot.elementId == 0)
    {
        return;
    }

    slots.push_back(slot);

    auto it = activeViews.find(slot.elementId);
    if (it == activeViews.end())
    {
        ActiveNativeView view;
        view.slot = slot;
        view.seenThisSync = true;
        ensureAdapter(view);
        if (view.adapter != nullptr)
        {
            view.adapter->update(view.slot);
        }
        activeViews[slot.elementId] = std::move(view);
        return;
    }

    ActiveNativeView &view = it->second;
    const bool identityChanged = view.slot.nativeType != slot.nativeType || view.slot.viewId != slot.viewId;
    if (identityChanged && view.adapter != nullptr)
    {
        view.adapter->unmount();
        view.adapter.reset();
    }

    view.slot = slot;
    view.seenThisSync = true;
    ensureAdapter(view);
    if (view.adapter != nullptr)
    {
        view.adapter->update(view.slot);
    }
}

void NativeViewRegistry::endSync()
{
    std::vector<uint64_t> removedIds;
    for (auto &entry : activeViews)
    {
        if (entry.second.seenThisSync)
        {
            continue;
        }

        if (entry.second.adapter != nullptr)
        {
            entry.second.adapter->unmount();
        }
        removedIds.push_back(entry.first);
    }

    for (uint64_t id : removedIds)
    {
        activeViews.erase(id);
    }

    if (activeViews.find(capturedElementId) == activeViews.end())
    {
        capturedElementId = 0;
    }
    if (activeViews.find(focusedElementId) == activeViews.end())
    {
        focusedElementId = 0;
    }
}

const std::vector<NativeViewSlot> &NativeViewRegistry::getSlots() const
{
    return slots;
}

uint64_t NativeViewRegistry::hitTest(float x, float y) const
{
    for (auto it = slots.rbegin(); it != slots.rend(); ++it)
    {
        if (!it->visible || it->rect.width <= 0.0f || it->rect.height <= 0.0f)
        {
            continue;
        }

        if (it->rect.Contains(x, y))
        {
            return it->elementId;
        }
    }

    return 0;
}

bool NativeViewRegistry::dispatchTo(uint64_t elementId, NativeViewInputEvent event)
{
    auto it = activeViews.find(elementId);
    if (it == activeViews.end() || it->second.adapter == nullptr)
    {
        return false;
    }

    const NativeViewSlot &slot = it->second.slot;
    event.elementId = slot.elementId;
    event.viewId = slot.viewId;
    event.localX = event.x - slot.rect.x;
    event.localY = event.y - slot.rect.y;
    return it->second.adapter->dispatchInput(event);
}

bool NativeViewRegistry::dispatchMouseEvent(const MouseEvent &event)
{
    uint64_t targetId = capturedElementId;

    if (event.type == MouseEvent::Type::Down)
    {
        targetId = hitTest(event.x, event.y);
        capturedElementId = targetId;
        focusedElementId = targetId;
    }
    else if (targetId == 0)
    {
        targetId = hitTest(event.x, event.y);
    }

    if (targetId == 0)
    {
        return false;
    }

    NativeViewInputEvent inputEvent;
    inputEvent.x = event.x;
    inputEvent.y = event.y;
    inputEvent.button = static_cast<int>(event.button);
    if (event.type == MouseEvent::Type::Down)
    {
        inputEvent.type = NativeViewInputEvent::Type::PointerDown;
    }
    else if (event.type == MouseEvent::Type::Up)
    {
        inputEvent.type = NativeViewInputEvent::Type::PointerUp;
    }
    else
    {
        inputEvent.type = NativeViewInputEvent::Type::PointerMove;
    }

    const bool handled = dispatchTo(targetId, inputEvent);
    if (event.type == MouseEvent::Type::Up)
    {
        capturedElementId = 0;
    }
    return handled;
}

bool NativeViewRegistry::dispatchMouseWheelEvent(const MouseWheelEvent &event, float x, float y)
{
    uint64_t targetId = hitTest(x, y);
    if (targetId == 0)
    {
        return false;
    }

    NativeViewInputEvent inputEvent;
    inputEvent.type = NativeViewInputEvent::Type::MouseWheel;
    inputEvent.x = x;
    inputEvent.y = y;
    inputEvent.delta = event.delta;
    return dispatchTo(targetId, inputEvent);
}

bool NativeViewRegistry::dispatchKeyEvent(const KeyEvent &event)
{
    if (focusedElementId == 0)
    {
        return false;
    }

    NativeViewInputEvent inputEvent;
    inputEvent.type = event.isPressed ? NativeViewInputEvent::Type::KeyDown : NativeViewInputEvent::Type::KeyUp;
    inputEvent.keyCode = event.keyCode;
    inputEvent.altDown = event.altDown;
    inputEvent.controlDown = event.controlDown;
    inputEvent.shiftDown = event.shiftDown;
    inputEvent.metaDown = event.metaDown;
    return dispatchTo(focusedElementId, inputEvent);
}

REGISTER_UI_COMPONENT(NativeViewRegistry, ServicePhase::Logic);
