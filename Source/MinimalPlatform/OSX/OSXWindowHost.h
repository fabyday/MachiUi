#pragma once
#include "../../Core/IWindowHost.h"
#include <vector>

class ServiceProvider;

class OSXWindowHost : public IWindowHost
{
    std::vector<IWindow *> windowLists;

public:
    OSXWindowHost();
    virtual ~OSXWindowHost() override;

    // IService interface implementation
    void onInit(ServiceProvider *provider) override;

    // IWindowHost interface implementation
    IWindow *requestWindow() override;
};