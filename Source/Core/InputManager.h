#pragma once
#include "IService.h"
#include "Types.h"
#include <optional>
#include <string>
#include "../Common/Code.h"
#include "MessageBus.h"
#include <unordered_map>

#include "../Common/Hash.h"

#define MachiArgsHashByName(NAME_STR) NAME_STR, compileTimeHash(NAME_STR)

class InputManager : public IService
{

public:
    virtual ~InputManager() = default;

    std::unordered_map<uint64_t, MessageBus> channels;

    virtual void onInit(ServiceProvider *engine) override;

    /**
     * Use "Common/Hash.h" Hashfunction
     */
    MachiCode getChannel(const std::string &channelName);
    MachiCode getChannel(const uint64_t channelHash);

    /**
     * create MessageBus Channel
     * Use "Common/Hash.h" Hashfunction
     *
     */
    MachiCode createChannel(const uint64_t channelHash);
    MachiCode createChannel(const std::string &channelName);
    /**
     * create MessageBus Channel with custom hash
     */
    MachiCode createChannel(const std::string &channelName, const uint64_t channelHash);
};