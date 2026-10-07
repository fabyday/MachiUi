#pragma once
#include <vector>
#include "RenderCommand.h"
#include <variant>
class RenderQueue
{
public:
    
    RenderQueue(size_t initialCapacity = 100)
        : m_commands()
    {
        m_commands.reserve(initialCapacity);
    }

    void reserve(size_t capacity)
    {
        m_commands.reserve(capacity);
    }

    void recordCommand(ViewId id, Color color)
    {
        m_commands.push_back({CommandType::DrawRect, id, color});
    }

    void recordCommand(ViewId id, Rect bounds, Color color)
    {
        m_commands.push_back({CommandType::DrawRect, id, color, bounds, true});
    }

    void recordText(ViewId id, Rect bounds, const char *text)
    {
        m_commands.push_back({CommandType::DrawText, id, TextData{text}, bounds, true});
    }

    void recordCommand(const RenderCommand &command) { m_commands.push_back(command); }

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