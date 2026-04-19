#include "TaskScheduler.h"
#include "ServiceRegistry.h"
#include "Core/UiEngine.h"
#include <chrono>
#include "ServiceProvider.h"

void TaskScheduler::onInit(ServiceProvider *engine)
{
    this->timer = engine->getService<ITimer>();
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

void TaskScheduler::pushRenderTask(const RenderCommand &cmd)
{
    


    


}

REGISTER_UI_COMPONENT(TaskScheduler, ServicePhase::Logic)