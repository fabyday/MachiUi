#include "ViewManger.h"
#include "UiEngine.h"
#include "ServiceProvider.h"
#include "Core/LogManager.h"

void ViewManager::onInit(ServiceProvider *provider)
{
    logger = provider->getService<LogManager>()->getLogger();
    winHost = provider->getService<IWindowHost>();
    if (!winHost)
    {
        logger->LogError("ViewManager initialization failed: IWindowHost service not found.");
        return;
    }

    logger->LogInfo("ViewManager initialized successfully.");
}

bool ViewManager::validate(ViewId id)
{
    auto it = this->viewInfoMap.find(id);
    if (it == viewInfoMap.end() || !it->second.isAlive)
    {
        return false;
    }

    return true;
}

void ViewManager::attachView(ViewId view, ViewId parent)
{
    if (!validate(view) || !validate(parent))
    {
        return;
    }

    viewInfoMap[view].parentId = parent;
    viewInfoMap[view].isWindow = false;
}

void ViewManager::detachView(ViewId view)
{
    if (!validate(view))
    {
        return;
    }

    viewInfoMap[view].parentId = 0;
}

void ViewManager::destroyView(ViewId view)
{
    auto it = viewInfoMap.find(view);
    if (it == viewInfoMap.end())
    {
        return;
    }

    it->second.isAlive = false;
    windowMap.erase(view);
}

ViewId ViewManager::createView(ViewId parentId)
{
    if (parentId > 0 && !validate(parentId))
    {
        return 0;
    }

    ViewId id = generateUniqueId();
    IWindow *targetNativeWindow = nullptr;

    if (parentId <= 0)
    {
        if (this->winHost != nullptr)
        {
            targetNativeWindow = this->winHost->requestWindow();
            if (!targetNativeWindow && logger != nullptr)
            {
                logger->LogError("Failed to create a new native window.");
            }
        }
    }

    viewInfoMap[id] = ViewInfo{
        id,
        parentId,
        targetNativeWindow != nullptr,
        true};

    if (targetNativeWindow != nullptr)
    {
        windowMap[id] = targetNativeWindow;
    }

    return id;
}

IWindow *ViewManager::getWindowByViewId(ViewId id)
{
    auto windowIt = windowMap.find(id);
    if (windowIt != windowMap.end())
    {
        return windowIt->second;
    }

    auto viewIt = viewInfoMap.find(id);
    if (viewIt == viewInfoMap.end() || viewIt->second.parentId == 0)
    {
        return nullptr;
    }

    return getWindowByViewId(viewIt->second.parentId);
}

ViewInfo *ViewManager::getViewInfo(ViewId id)
{
    auto it = viewInfoMap.find(id);
    if (it == viewInfoMap.end())
    {
        return nullptr;
    }

    return &it->second;
}

REGISTER_UI_COMPONENT(ViewManager, ServicePhase::Logic)
