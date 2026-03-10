#include "../../Core/ComponentRegistry.h"
#include "OSXWindowHost.h"
#include "OSXWindow.h"

void OSXWindowHost::onInit(UiEngine *engine)
{


}


IWindow* OSXWindowHost::requestWindow()
{
    return create_window();
    return nullptr;
}

REGISTER_UI_COMPONENT(OSXWindowHost, ServicePhase::System);