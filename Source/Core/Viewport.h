

#pragma once

#include "Types.h"

class Viewport
{
private:
    Rect bounds;
    float opacity = 1.0f;

public:
    void setBounds(int x, int y, int width, int height);
    void setBounds(Rect &rect);
};