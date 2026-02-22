#pragma once
#include "../Core/Types.h"
enum class CommandType
{
    DrawRect,
    DrawText,
    SetClip,
    PushLayer
};

struct RenderCommand
{
    CommandType type;
    Rect bounds;
    Color color;
};