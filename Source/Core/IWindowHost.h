#pragma once

#include "IService.h"
#include "IWindow.h"



class IWindowHost : public IService
{
public:
    virtual ~IWindowHost() = default;
    virtual IWindow *requestWindow() = 0;
};