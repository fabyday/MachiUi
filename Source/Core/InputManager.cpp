#pragma once
#include "IService.h"
#include "Types.h"
#include <optional>
#include "InputManager.h"
#include "../Common/Hash.h"
#include "ServiceRegistry.h"

void InputManager::onInit(ServiceProvider *engine)
{
}

MachiCode InputManager::getChannel(const std::string &channelName)
{
    return this->getChannel(MachiCompileTimeHash(channelName));
}

MachiCode InputManager::getChannel(const uint64_t channelName)
{
    auto it = this->channels.find(channelName);
    if (it == this->channels.end())
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_INVALID_PARAM, "Channel not found");
    }

    return MAKE_MACHI_SUCCESS;
}

MachiCode InputManager::createChannel(const uint64_t channelHash)
{

    if (this->channels.find(channelHash) != this->channels.end())
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_ALREADY_EXISTS, "Channel already exists");
    }

    this->channels[channelHash] = MessageBus(std::to_string(channelHash), channelHash);

    return MAKE_MACHI_SUCCESS;
}

MachiCode InputManager::createChannel(const std::string &channelName)
{
    return this->createChannel(MachiCompileTimeHash(channelName));
}

MachiCode InputManager::createChannel(const std::string &channelName, const uint64_t channelHash)
{
    if (this->channels.find(channelHash) != this->channels.end())
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_ALREADY_EXISTS, "Channel already exists");
    }

    this->channels[channelHash] = MessageBus(channelName, channelHash);

    return MAKE_MACHI_SUCCESS;
}

REGISTER_UI_COMPONENT(InputManager, ServicePhase::System);
