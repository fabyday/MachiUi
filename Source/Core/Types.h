#pragma once

struct Color {
    float r, g, b, a;
    
    static Color White() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static Color Red()   { return {1.0f, 0.0f, 0.0f, 1.0f}; }
};

struct Rect {
    float x, y, width, height;

    bool Contains(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }
};