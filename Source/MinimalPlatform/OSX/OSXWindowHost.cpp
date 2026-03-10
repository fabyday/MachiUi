#include "../../Core/ComponentRegistry.h"
#include "OSXWindowHost.h"
#include "OSXWindow.h"

void OSXWindowHost::onInit(UiEngine *engine)
{


}
OSXWindowHost::OSXWindowHost()
{
}
OSXWindowHost::~OSXWindowHost()
{
}

IWindow* OSXWindowHost::requestWindow()
{
    return createWindow();
}

REGISTER_UI_COMPONENT(OSXWindowHost, ServicePhase::System);