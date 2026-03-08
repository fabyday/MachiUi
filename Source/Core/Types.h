#pragma once

struct Color
{
    float r, g, b, a;

    static Color White() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static Color Red() { return {1.0f, 0.0f, 0.0f, 1.0f}; }
};

struct Rect
{
    float x, y, width, height;

    bool Contains(float px, float py) const
    {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }
};

struct Event
{
};

struct InputEvent
{
    enum class Type
    {
        None,
        KeyDown,
        KeyUp,
        MouseDown,
        MouseUp,
        MouseMove,
        MouseWheel,
        TouchStart,
        TouchMove,
        TouchEnd
    } type;

    union
    {
        struct
        {
            int keyCode;
            bool altDown;
            bool controlDown;
            bool shiftDown;
        } key;

        struct
        {
            float x;
            float y;
            int button; // 0: Left, 1: Middle, 2: Right
        } mouse;

        struct
        {
            float delta;
        } wheel;
    };

};

/// Input Event Type

struct KeyEvent
{
    int keyCode;
    bool isPressed;
    bool altDown;
    bool controlDown;
    bool shiftDown;
    bool metaDown;
};

struct MouseEvent
{
};

// Nah...
// struct TouchEvent{

// };
