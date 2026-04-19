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

ViewId ViewManager::createView(ViewId parentId)
{

    IWindow *targetNativeWindow = nullptr;

    if (parentId <= 0)
    {
        // Create a new native window
        targetNativeWindow = this->winHost->requestWindow();
        if (!targetNativeWindow)
        {
            logger->LogError("Failed to create a new native window.");
            return 0; // Invalid ID
        }
    }
    else
    {

        if (this->validate(parentId))
        {
            // Create a child view (not a window)
            
        }
    }

    return ViewId();
}

REGISTER_UI_COMPONENT(ViewManager, ServicePhase::Logic)