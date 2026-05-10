#pragma once
#include "IService.h"
#include "Types.h"
#include "EventBus.h"
#include "MessageBus.h"

#include <cstdint>
#include <string>
#include <unordered_set>

#include "../Common/Code.h"
#include "../Common/Hash.h"

#define MachiArgsHashByName(NAME_STR) NAME_STR, compileTimeHash(NAME_STR)

namespace InputChannels
{
    constexpr const char *DefaultInputName = "DefaultInputChannel";
    constexpr const char *WindowEventName = "WindowEventChannel";
    constexpr uint64_t DefaultInput = compileTimeHash(DefaultInputName);
    constexpr uint64_t WindowEvent = compileTimeHash(WindowEventName);
}

class InputManager : public IService
{
private:
    EventBus *eventBus = nullptr;
    std::unordered_set<int> pressedKeys;

    MachiCode publishInputMessage(const IMessage &message);
    MachiCode publishWindowMessage(const IMessage &message);

public:
    virtual ~InputManager() = default;

    virtual void onInit(ServiceProvider *engine) override;

    MachiCode ensureDefaultChannels();

    MachiCode emitKeyEvent(const KeyEvent &event);
    MachiCode emitMouseEvent(const MouseEvent &event);
    MachiCode emitMouseWheelEvent(const MouseWheelEvent &event);
    MachiCode emitWindowEvent(const WindowEvent &event);

    bool isKeyDown(int keyCode) const;

    MachiCode subscribeInput(MessageBus::MessageHandler handler);
    MachiCode subscribeWindowEvent(MessageBus::MessageHandler handler);

    MessageBus *getInputChannel();
    MessageBus *getWindowEventChannel();

    /**
     * Legacy channel helpers. Prefer EventBus for plugin/custom channels.
     */
    MachiCode getChannel(const std::string &channelName);
    MachiCode getChannel(const uint64_t channelHash);
    MachiCode createChannel(const uint64_t channelHash);
    MachiCode createChannel(const std::string &channelName);
    MachiCode createChannel(const std::string &channelName, const uint64_t channelHash);
};
