#pragma once
#include "Core/Types.h"
#include <cstdint>
#include <variant>

enum class CommandType
{
    DrawRect,
    DrawText,
    SetClip,
    PushLayer
};
typedef  uint64_t ViewId;
typedef struct TextureId
{
    uint32_t id;
} TextureId;

typedef struct TextData
{
    const char *content;
} TextData;

struct RenderCommand
{
    CommandType type;
    ViewId target;
    std::variant<Color, TextureId, TextData> data;
};

// Vertex Packet Structure
struct UiVertex
{
    float position[2];
    float uv[2];
    uint32_t color;
    uint32_t mode;
    uint32_t textureIndex;
};