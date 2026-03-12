#include "Win32WindowHost.h"
#include "../../Core/ComponentRegistry.h"
#include "osdeps.h"

Win32WindowHost::Win32WindowHost()
{
}

Win32WindowHost::~Win32WindowHost()
{
}

// IService interface implementation
void Win32WindowHost::onInit(UiEngine *engine)
{
}

// For Standalone Mode
IWindow *Win32WindowHost::requestWindow()
{
    IWindow *result = nullptr;
    if (winPool.size())
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

REGISTER_UI_COMPONENT(Win32WindowHost, ServicePhase::System);
