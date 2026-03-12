#include <Core/DefaultTimer.h>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

class TimerTest : public ::testing::Test {

protected:
  DefaultTimer timer;
  void SetUp() override { timer.onInit(nullptr); }
};

// delegate
class IClock {
public:
  virtual std::chrono::steady_clock::time_point now() = 0;
};

class MockClock : public IClock {
public:
  std::chrono::steady_clock::time_point fixedTime;

  std::chrono::steady_clock::time_point now() override { return fixedTime; }
};

TEST_F(TimerTest, DeltaTimeCalculation) {

  timer.tick();

  // 100milsec sleep for checking DeltaTime
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  timer.tick();

  double deltaTime = timer.getDeltaTime();

  EXPECT_NEAR(deltaTime, 0.1, 0.02);
}
