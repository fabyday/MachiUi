#pragma once
#include <chrono>
#include "ITimer.h"

class DefaultTimer : public ITimer
{
private:
    using Clock = std::chrono::high_resolution_clock;
    using Second = std::chrono::duration<double>;
    bool _isPaused = false;
    double _deltaTime = 0.0;
    double _totalActiveTime = 0.0;
    uint64_t totalMS = 0;

    Nanoseconds _startTime;    // 엔진 시작 시점
    Nanoseconds _lastTickTime; // 마지막 tick 호출 시점

public:
    DefaultTimer();
    virtual ~DefaultTimer() override = default;

    // IService interface
    virtual void onInit(UiEngine *engine) override;

    // ITimer interface
    virtual void setPaused(bool paused) override;
    virtual bool isPaused() const override { return _isPaused; }

    virtual void tick() override;

protected:
    virtual Nanoseconds now() const;

public:
    virtual double getDeltaTime() override { return _deltaTime; }
    virtual double getTotalActiveTime() const override { return _totalActiveTime; }
    virtual double getAbsoluteTime() const override;

    virtual uint64_t getTotalTime() override;
};