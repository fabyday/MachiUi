#include "EventBus.h"
#include "ServiceRegistry.h"

#include <utility>

void EventBus::onInit(ServiceProvider *provider)
{
}

MachiCode EventBus::createChannel(uint64_t channelHash)
{
    return createChannel(std::to_string(channelHash), channelHash);
}

MachiCode EventBus::createChannel(const std::string &channelName)
{
    return createChannel(channelName, MachiHash(channelName));
}

MachiCode EventBus::createChannel(const std::string &channelName, uint64_t channelHash)
{
    if (channels.find(channelHash) != channels.end())
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_ALREADY_EXISTS, "Channel already exists");
    }

    channels.emplace(channelHash, std::make_unique<MessageBus>(channelName, channelHash));
    return MAKE_MACHI_SUCCESS;
}

MessageBus *EventBus::getChannel(uint64_t channelHash)
{
    auto it = channels.find(channelHash);
    if (it == channels.end())
    {
        return nullptr;
    }

    return it->second.get();
}

MessageBus *EventBus::getChannel(const std::string &channelName)
{
    return getChannel(MachiHash(channelName));
}

MachiCode EventBus::publish(uint64_t channelHash, const IMessage &message)
{
    auto *channel = getChannel(channelHash);
    if (channel == nullptr)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_INVALID_PARAM, "Channel not found");
    }

    return channel->pushMessage(message);
}

MachiCode EventBus::publish(const std::string &channelName, const IMessage &message)
{
    return publish(MachiHash(channelName), message);
}

MachiCode EventBus::subscribe(uint64_t channelHash, MessageBus::MessageHandler handler)
{
    auto *channel = getChannel(channelHash);
    if (channel == nullptr)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_INVALID_PARAM, "Channel not found");
    }

    return channel->subscribe(std::move(handler));
}

MachiCode EventBus::subscribe(const std::string &channelName, MessageBus::MessageHandler handler)
{
    return subscribe(MachiHash(channelName), std::move(handler));
}

REGISTER_UI_COMPONENT(EventBus, ServicePhase::System);
