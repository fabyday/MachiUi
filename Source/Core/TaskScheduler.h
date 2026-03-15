#pragma once

#include "Core/IService.h"
#include <functional>
#include <algorithm>
#include <vector>
#include <set>
#include "Core/ITimer.h"
class TaskScheduler : public IService
{
    std::multiset<std::function<void(void)>> taskQueue;
    ITimer *timer;
    struct Task
    {
        uint64_t execute_at;            // (current_time(absolute) + delay)
        std::function<bool()> callback; // bool(false: terminates, true: do next)

        // multiset 정렬 기준: 시간이 낮은 순(임박한 순)
        bool operator<(const Task &other) const
        {
            if (execute_at == other.execute_at)
                return this < &other; // 주소값으로 중복 방지
            return execute_at < other.execute_at;
        }
    };

public:
    // Managers should call this method.
    // mil-sec
    void postDelayTask(uint64_t delay, std::function<void(void)> callback);

    //
    void postPeriodicTask(uint64_t interval, uint64_t initialDelay, std::function<void(void)> callback);

    // run on engine
    void processReservedTask();

    void onInit(UiEngine *engine) override;

    void update();

private:
};
