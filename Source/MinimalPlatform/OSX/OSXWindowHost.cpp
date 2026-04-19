#include "Core/ServiceRegistry.h"
#include "OSXWindowHost.h"
#include "OSXWindow.h"

void OSXWindowHost::onInit(ServiceProvider *provider)
{
}
OSXWindowHost::OSXWindowHost()
{
}
OSXWindowHost::~OSXWindowHost()
{
    for (auto win : windowLists)
    {
        delete win;
    }
    windowLists.clear();
}

IWindow *OSXWindowHost::requestWindow()
{
    return createWindow();
}

REGISTER_UI_COMPONENT_AS(OSXWindowHost, IWindowHost, ServicePhase::System);