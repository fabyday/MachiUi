#include "MessageBus.h"

#include <utility>

const std::string &MessageBus::getName() const
{
    return name;
}

uint64_t MessageBus::getHash() const
{
    return hash;
}

bool MessageBus::hasMessage() const
{
    return !msgQueue.empty();
}

size_t MessageBus::messageCount() const
{
    return msgQueue.size();
}

MachiCode MessageBus::pushMessage(const IMessage &msg)
{
    msgBuffer.push_back(msg);
    msgQueue.push(msg);

    for (const auto &handler : handlers)
    {
        handler(msg);
    }

    return MAKE_MACHI_SUCCESS;
}

MachiCode MessageBus::pushMessage(IMessage *msg)
{
    if (msg == nullptr)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_NULL_PTR, "Message pointer is null");
    }

    return pushMessage(*msg);
}

MachiCode MessageBus::popMessage(IMessage &msg)
{
    if (msgQueue.empty())
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_INVALID_PARAM, "Message queue is empty");
    }

    msg = msgQueue.front();
    msgQueue.pop();
    return MAKE_MACHI_SUCCESS;
}

MachiCode MessageBus::getMessage()
{
    if (msgQueue.empty())
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_INVALID_PARAM, "Message queue is empty");
    }

    msgQueue.pop();
    return MAKE_MACHI_SUCCESS;
}

MachiCode MessageBus::subscribe(MessageHandler handler)
{
    if (!handler)
    {
        return MAKE_MACHI_ERR(MachiCodeEnum::MACHI_ERR_INVALID_PARAM, "Message handler is empty");
    }

    handlers.push_back(std::move(handler));
    return MAKE_MACHI_SUCCESS;
}
