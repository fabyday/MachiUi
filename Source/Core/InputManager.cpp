#include "InputManager.h"
#include "ServiceProvider.h"
#include "ServiceRegistry.h"

#include <utility>

void InputManager::onInit(ServiceProvider *engine)
{
    eventBus = engine->getService<EventBus>();
    ensureDefaultChannels();
}

MachiCode InputManager::ensureDefaultChannels()
{
    if (eventBus == nullptr)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_NULL_PTR, "EventBus service is not available");
    }

    auto code = eventBus->createChannel(InputChannels::DefaultInputName, InputChannels::DefaultInput);
    if (isMachiFailed(code) && code.code != MachiCodeEnum::MACHI_ERR_ALREADY_EXISTS)
    {
        return code;
    }

    code = eventBus->createChannel(InputChannels::WindowEventName, InputChannels::WindowEvent);
    if (isMachiFailed(code) && code.code != MachiCodeEnum::MACHI_ERR_ALREADY_EXISTS)
    {
        return code;
    }

    return MAKE_MACHI_SUCCESS;
}

MachiCode InputManager::publishInputMessage(const IMessage &message)
{
    if (eventBus == nullptr)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_NULL_PTR, "EventBus service is not available");
    }

    return eventBus->publish(InputChannels::DefaultInput, message);
}

MachiCode InputManager::publishWindowMessage(const IMessage &message)
{
    if (eventBus == nullptr)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_NULL_PTR, "EventBus service is not available");
    }

    return eventBus->publish(InputChannels::WindowEvent, message);
}

MachiCode InputManager::emitKeyEvent(const KeyEvent &event)
{
    if (event.isPressed)
    {
        pressedKeys.insert(event.keyCode);
    }
    else
    {
        pressedKeys.erase(event.keyCode);
    }

    return publishInputMessage(IMessage{event});
}

MachiCode InputManager::emitMouseEvent(const MouseEvent &event)
{
    return publishInputMessage(IMessage{event});
}

MachiCode InputManager::emitMouseWheelEvent(const MouseWheelEvent &event)
{
    return publishInputMessage(IMessage{event});
}

MachiCode InputManager::emitWindowEvent(const WindowEvent &event)
{
    return publishWindowMessage(IMessage{event});
}

bool InputManager::isKeyDown(int keyCode) const
{
    return pressedKeys.find(keyCode) != pressedKeys.end();
}

MachiCode InputManager::subscribeInput(MessageBus::MessageHandler handler)
{
    if (eventBus == nullptr)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_NULL_PTR, "EventBus service is not available");
    }

    return eventBus->subscribe(InputChannels::DefaultInput, std::move(handler));
}

MachiCode InputManager::subscribeWindowEvent(MessageBus::MessageHandler handler)
{
    if (eventBus == nullptr)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_NULL_PTR, "EventBus service is not available");
    }

    return eventBus->subscribe(InputChannels::WindowEvent, std::move(handler));
}

MessageBus *InputManager::getInputChannel()
{
    if (eventBus == nullptr)
    {
        return nullptr;
    }

    return eventBus->getChannel(InputChannels::DefaultInput);
}

MessageBus *InputManager::getWindowEventChannel()
{
    if (eventBus == nullptr)
    {
        return nullptr;
    }

    return eventBus->getChannel(InputChannels::WindowEvent);
}

MachiCode InputManager::getChannel(const std::string &channelName)
{
    return this->getChannel(MachiHash(channelName));
}

MachiCode InputManager::getChannel(const uint64_t channelName)
{
    if (eventBus == nullptr)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_NULL_PTR, "EventBus service is not available");
    }

    if (eventBus->getChannel(channelName) == nullptr)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_INVALID_PARAM, "Channel not found");
    }

    return MAKE_MACHI_SUCCESS;
}

MachiCode InputManager::createChannel(const uint64_t channelHash)
{
    if (eventBus == nullptr)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_NULL_PTR, "EventBus service is not available");
    }

    return eventBus->createChannel(channelHash);
}

MachiCode InputManager::createChannel(const std::string &channelName)
{
    if (eventBus == nullptr)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_NULL_PTR, "EventBus service is not available");
    }

    return eventBus->createChannel(channelName);
}

MachiCode InputManager::createChannel(const std::string &channelName, const uint64_t channelHash)
{
    if (eventBus == nullptr)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_NULL_PTR, "EventBus service is not available");
    }

    return eventBus->createChannel(channelName, channelHash);
}

REGISTER_UI_COMPONENT(InputManager, ServicePhase::System);
