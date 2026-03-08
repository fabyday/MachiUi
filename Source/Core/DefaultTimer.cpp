#include "DefaultTimer.h"
#include "ComponentRegistry.h"

DefaultTimer::DefaultTimer()
    : _startTime(Clock::now()), _lastTickTime(Clock::now())
{
}

void DefaultTimer::onInit(UiEngine *engine)
{
    // 초기화 시점의 시간 기록
    _startTime = Clock::now();
    _lastTickTime = _startTime;
}

void DefaultTimer::setPaused(bool paused)
{
    if (_isPaused == paused)
        return;

    _isPaused = paused;

    if (!_isPaused)
    {
        // 일시정지 해제 시, 마지막 틱 시간을 현재로 강제 갱신합니다.
        // 이렇게 해야 Resume 직후 첫 tick에서 deltaTime이 거대하게 튀지 않습니다.
        _lastTickTime = Clock::now();
    }
}

void DefaultTimer::tick()
{
    auto currentTime = Clock::now();

    if (_isPaused)
    {
        // 일시정지 중에는 시간 변화량이 0입니다.
        _deltaTime = 0.0;
    }
    else
    {
        // 현재 시간과 마지막 틱 시간의 차이를 초 단위로 계산
        Second elapsed = currentTime - _lastTickTime;
        _deltaTime = elapsed.count();
        _totalActiveTime += _deltaTime;
    }

    _lastTickTime = currentTime;
}

double DefaultTimer::getAbsoluteTime() const
{
    // 일시정지 여부와 상관없이 엔진 시작 후 실제 흐른 시간
    Second elapsed = Clock::now() - _startTime;
    return elapsed.count();
}

REGISTER_UI_COMPONENT(DefaultTimer, ServicePhase::System);
