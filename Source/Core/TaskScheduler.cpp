#include "TaskScheduler.h"
#include "ComponentRegistry.h"
#include "Core/UiEngine.h"
#include <chrono>

void TaskScheduler::onInit(UiEngine *engine)
{
    this->timer = engine->GetService<ITimer>();
}

void TaskScheduler::processReservedTask()
{
}

void TaskScheduler::postPeriodicTask(uint64_t interval, uint64_t initialDelay, std::function<void(void)> callback)
{
}

void TaskScheduler::postDelayTask(uint64_t delay, std::function<void(void)> callback)
{

    uint64_t targetTime = this->timer->getTotalTime() + delay;
    // this->taskQueue.insert({targetTime, callback});
}

REGISTER_UI_COMPONENT(TaskScheduler, ServicePhase::Logic)