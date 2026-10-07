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
    auto window = createWindow();
    windowLists.push_back(window);
    return window;
}

REGISTER_UI_COMPONENT_AS(OSXWindowHost, IWindowHost, ServicePhase::System);
void OSXWindowHost::update()
{
    for (auto window : windowLists) if (!window->shouldClose()) window->update();
}
