#pragma once
#include <vector>
#include "RenderCommand.h"

class RenderQueue
{
public:
    void PushDrawRect(Rect bounds, Color color)
    {
        m_commands.push_back({CommandType::DrawRect, bounds, color});
    }

    void Clear()
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