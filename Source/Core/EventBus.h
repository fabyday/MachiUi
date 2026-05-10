#pragma once

#include "IService.h"
#include "MessageBus.h"
#include "../Common/Code.h"
#include "../Common/Hash.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

class EventBus : public IService
{
public:
    virtual ~EventBus() = default;

    void onInit(ServiceProvider *provider) override;

    MachiCode createChannel(uint64_t channelHash);
    MachiCode createChannel(const std::string &channelName);
    MachiCode createChannel(const std::string &channelName, uint64_t channelHash);

    MessageBus *getChannel(uint64_t channelHash);
    MessageBus *getChannel(const std::string &channelName);

    MachiCode publish(uint64_t channelHash, const IMessage &message);
    MachiCode publish(const std::string &channelName, const IMessage &message);

    MachiCode subscribe(uint64_t channelHash, MessageBus::MessageHandler handler);
    MachiCode subscribe(const std::string &channelName, MessageBus::MessageHandler handler);

private:
    std::unordered_map<uint64_t, std::unique_ptr<MessageBus>> channels;
};
