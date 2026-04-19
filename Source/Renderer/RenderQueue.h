#pragma once
#include <vector>
#include "RenderCommand.h"
#include <variant>
class RenderQueue
{
public:
    
    RenderQueue(size_t initialCapacity = 100)
        : m_commands(initialCapacity)
    {
    }

    void reserve(size_t capacity)
    {
        m_commands.reserve(capacity);
    }

    void recordCommand(ViewId id, Color color)
    {
        m_commands.push_back({CommandType::DrawRect, id, color});
    }

    void clearCommands()
    {
        m_commands.clear();
    }

    const std::vector<RenderCommand> &GetCommands() const
    {
        return m_commands;
    }

private:
    std::vector<RenderCommand> m_commands;
};