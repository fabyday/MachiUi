#pragma once
#include <vector>;

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

public:
    MessageBus(std::string name, uint64_t hash) : name(name), hash(hash) {}
    MachiCode pushMessage(IMessage *msg);
    MachiCode getMessage();
};