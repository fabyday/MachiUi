#include "DefaultTimer.h"
#include "ComponentRegistry.h"
#include <chrono>

DefaultTimer::DefaultTimer()
    : _startTime(this->now()), _lastTickTime(this->now()) {}

void DefaultTimer::onInit(UiEngine *engine)
{
  // 초기화 시점의 시간 기록
  _startTime = this->now();
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
    _lastTickTime = this->now();
  }
}

void DefaultTimer::tick()
{
  auto currentTime = this->now();
  auto frameDiff = currentTime - _lastTickTime;

  if (_isPaused)
  {
    _deltaTime = 0.0;
    this->_totalActiveTime += frameDiff;
  }
  else
  {
    auto deltaSec = frameDiff;
    // this->_deltaTime = deltaSec.count();

    this->_totalActiveTime += deltaSec;
    // std::chrono::duration_cast<std::chrono::milliseconds>(frameDiff)
    //     .count();
  }

  _lastTickTime = currentTime;
}

DefaultTimer::Nanoseconds DefaultTimer::now() const
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

double DefaultTimer::getAbsoluteTime() const
{
  // 일시정지 여부와 상관없이 엔진 시작 후 실제 흐른 시간
  auto elapsed = this->now() - _startTime;
  return NsToMs(elapsed);
}

uint64_t DefaultTimer::getTotalTime() { return this->totalMS; }

REGISTER_UI_COMPONENT(DefaultTimer, ServicePhase::System);
