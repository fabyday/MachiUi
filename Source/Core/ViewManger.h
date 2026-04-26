#pragma once
#include <unordered_map>
#include "IService.h"
#include "IWindowHost.h"
#include "../Common/typedef.h"
#include "ILogger.h"
#include <cstdint>

class ServiceProvider;

struct ViewInfo
{
    ViewId id;
    ViewId parentId;
    bool isWindow;
    bool isAlive;
};



/**
 * View manager do...
 * 뷰 매니저가 무엇이냐
 * 
 */
class ViewManager : public IService
{
private:
    IWindowHost *winHost;
    ILogger *logger;

    std::unordered_map<ViewId, ViewInfo> viewInfoMap;
    std::unordered_map<ViewId, IWindow *> windowMap;

protected:
    ViewId generateUniqueId()
    {
        static ViewId currentId = 1; // Start from 1, reserve 0 for invalid ID
        return currentId++;
    }

public:
    // inherited
    void onInit(ServiceProvider *provider) override;

    //
    bool validate(ViewId id);
    void attachView(ViewId view, ViewId parent);
    void detachView(ViewId view);
    void destroyView(ViewId view);
    ViewId createView(ViewId parent = 0);
    IWindow *getWindowByViewId(ViewId id);

    // View Controll
    void handleMouseUp(ViewId id, int x, int y);
    void handleMouseDown(ViewId id, int x, int y);
    void handleMouseMove(ViewId id, int x, int y);
    void handleMouseEnter(ViewId id);
    void handleMouseLeave(ViewId id);



    
};
