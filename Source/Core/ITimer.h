#pragma once
#include "IService.h"

class ITimer : public IService
{

public:
    virtual ~ITimer() = 0;

    virtual void onInit(UiEngine *engine) = 0;
    // 일시정지 제어
    virtual void setPaused(bool paused) = 0;
    virtual bool isPaused() const = 0;
    
    virtual void tick() = 0;

    virtual double getDeltaTime() = 0;
    virtual double getTotalActiveTime() const = 0;
    virtual double getAbsoluteTime() const = 0;
};