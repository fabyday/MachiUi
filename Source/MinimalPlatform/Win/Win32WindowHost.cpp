#include "Win32WindowHost.h"
#include "../../Core/ServiceRegistry.h"
#include "osdeps.h"

Win32WindowHost::Win32WindowHost()
{
}

Win32WindowHost::~Win32WindowHost()
{
}

// IService interface implementation
void Win32WindowHost::onInit(ServiceProvider *provider)
{
}

// For Standalone Mode
IWindow *Win32WindowHost::requestWindow()
{
    IWindow *result = nullptr;
    if (winPool.empty())
    {
        result = createWindow();
        if (result == nullptr)
        {
            return result;
        }
        winPool.push_back(result);
    }
    return result;
}

REGISTER_UI_COMPONENT_AS(Win32WindowHost, IWindowHost, ServicePhase::System);
