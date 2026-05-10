#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include <queue>
#include "Message.h"
#include "../Common/Code.h"

/**
 *
 */
class MessageBus
{
private:
    std::string name;
    uint64_t hash;
    std::vector<IMessage> msgBuffer;
    std::queue<IMessage> msgQueue;
    std::vector<std::function<void(const IMessage &)>> handlers;

public:
    using MessageHandler = std::function<void(const IMessage &)>;

    MessageBus(std::string name, uint64_t hash) : name(name), hash(hash) {}
    const std::string &getName() const;
    uint64_t getHash() const;
    bool hasMessage() const;
    size_t messageCount() const;

    MachiCode pushMessage(const IMessage &msg);
    MachiCode pushMessage(IMessage *msg);
    MachiCode popMessage(IMessage &msg);
    MachiCode getMessage();
    MachiCode subscribe(MessageHandler handler);
};
