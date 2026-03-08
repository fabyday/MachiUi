#pragma once

#include "Viewport.h"
#include <memory>
#include <vector>
class Window
{
private:
    std::vector<std::unique_ptr<Viewport>> viewports;

public:
    void addViewport(std::unique_ptr<Viewport> viewport);
};