#pragma once

#include "IService.h"
#include "IWindow.h"

class IWindowHost : public IService
{
public:
    virtual ~IWindowHost() = default;
    virtual IWindow *requestWindow() = 0;

    /**
     * Updates raw input(like a OS dependant Event, Msg or key Inputs)
     */
    virtual void update() = 0;
};