#pragma once

#include <variant>
#include "Core/Types.h"




/**
 * Message Wrapper
 */
struct IMessage
{
    std::variant<KeyEvent, MouseButton, MouseWheelEvent, MouseEvent, WindowEvent> msg;
};
