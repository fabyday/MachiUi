#include "ViewManger.h"
#include "UiEngine.h"

void ViewManager::onInit(UiEngine *engine)
{
    winHost = engine->GetService<IWindowHost>();
}

bool ViewManager::validate(ViewId id)
{
    auto it = this->viewInfoMap.find(id);
    if (it == viewInfoMap.end() || !it->second.isAlive)
    {
        return false;
    }
    if (!it->second.isWindow)
    {
        return this->validate(it->second.parentId);
    }

    return false;
}

void ViewManager::attachView(ViewId view, ViewId parent)
{
}

void ViewManager::detachView(ViewId view)
{
}

void ViewManager::destroyView(ViewId view)
{
}

ViewId ViewManager::createView()
{
    IWindow *win = this->winHost->requestWindow();
    
    return ViewId();
}
