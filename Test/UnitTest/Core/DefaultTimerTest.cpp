#include <Core/DefaultTimer.h>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

class TestTimer : public DefaultTimer
{
public:
  uint64_t fakeNow = 0;

protected:
  virtual Nanoseconds now() const override
  {

    return fakeNow;
  }
};




class TimerTest : public ::testing::Test
{
protected:
  TestTimer timer;
  void SetUp() override
  {
    timer.onInit(nullptr);
    timer.fakeNow = 0_s;
  }
  void elapse(uint64_t duration)
  {
    timer.fakeNow += duration;
    timer.tick();
  }
};

TEST_F(TimerTest, DeltaTimeCalculation)
{
  elapse(0_s);
  EXPECT_NEAR(timer.getDeltaTime(), 0.0, 0.0001);

  elapse(1_s);
  EXPECT_NEAR(timer.getDeltaTime(), 1.0, 0.0001);

  elapse(16.6_ms);
  EXPECT_NEAR(timer.getDeltaTime(), 0.0166, 0.0001);
}



