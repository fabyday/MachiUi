#pragma once
#include "IService.h"
#include <cstdint>

class ITimer : public IService
{

public:
    virtual ~ITimer() = default;

    virtual void onInit(UiEngine *engine) = 0;
    // 일시정지 제어
    virtual void setPaused(bool paused) = 0;
    virtual bool isPaused() const = 0;

    // this method is called by engine 
    virtual void tick() = 0;

    virtual double getDeltaTime() = 0;
    virtual double getTotalActiveTime() const = 0;
    virtual double getAbsoluteTime() const = 0;

    virtual uint64_t getTotalTimeMS() = 0;
    // virtual double getCurrentTime();
};