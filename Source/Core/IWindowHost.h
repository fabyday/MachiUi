

#include "IService.h"
#include "IWindow.h"
#include "Viewport.h"
class IWindowHost : public IService
{

    IWindow *requestWindow();
};