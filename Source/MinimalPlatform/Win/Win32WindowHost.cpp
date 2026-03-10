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
    return createWindow();
}

REGISTER_UI_COMPONENT(Win32WindowHost, ServicePhase::System);
