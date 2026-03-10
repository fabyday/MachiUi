

#include "IService.h"
#include "IWindow.h"
#include "Viewport.h"
class IWindowHost : public IService
{
public:
    virtual ~IWindowHost() = default;
    virtual IWindow *requestWindow() = 0;
};