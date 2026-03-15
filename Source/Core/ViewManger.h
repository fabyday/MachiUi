#pragma once
#include <unordered_map>
#include "IService.h"
#include "IWindowHost.h"
#include <cstdint>
typedef uint32_t ViewId;

class UiEngine;

struct ViewInfo
{
    ViewId id;
    ViewId parentId;
    bool isWindow;
    bool isAlive;
};

class ViewManager : public IService
{
private:
    IWindowHost *winHost;
    std::unordered_map<ViewId, ViewInfo> viewInfoMap;
    std::unordered_map<ViewId, IWindow *> windowMap;

public:
    // inherited
    void onInit(UiEngine *engine) override;

    //
    bool validate(ViewId id);
    void attachView(ViewId view, ViewId parent);
    void detachView(ViewId view);
    void destroyView(ViewId view);
    ViewId createView();
};
