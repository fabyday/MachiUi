#pragma once
#include "../../Core/IWindowHost.h"
#include "../../Core/IWindow.h"
#include "../../Core/TaskScheduler.h"


#include <vector>

class Win32WindowHost : public IWindowHost
{
    void *opaque;
    std::vector<IWindow *> winPool;
    TaskScheduler *scheduler;

public:
    Win32WindowHost();
    virtual ~Win32WindowHost();

    // IService interface implementation
    void onInit(UiEngine *engine) override;

    // IWindowHost interface implementation
    IWindow *requestWindow() override;
};