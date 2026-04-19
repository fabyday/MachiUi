

#pragma once
#include "Common/typedef.h"
#include "Types.h"
// see @IWindow
//  this is Logical View, not a physical window. It can be attached to another view, and it can be detached from its parent view.

class View
{
private:
    ViewId id;
    ViewId parentId;
    float opacity = 1.0f;

    Rect bounds;
    bool isWindow; // true if this view is directly attached to a native window
    bool isAlive;

public:
    void setBounds(int x, int y, int width, int height);
    void setBounds(Rect &rect);
};
