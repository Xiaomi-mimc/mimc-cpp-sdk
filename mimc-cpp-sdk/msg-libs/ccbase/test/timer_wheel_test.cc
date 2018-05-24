#include <stdio.h>
#include <atomic>
#include <thread>
#include "gtestx/gtestx.h"
#include "ccbase/timer_wheel.h"

class TimerWheelTest : public testing::Test
{
protected:
  TimerWheelTest() {
  }
  void SetUp() {
  }
  void TearDown() {
  }
  ccb::TimerWheel tw_;
  int timers_ = 0;
  size_t count_ = 0;
};

TEST_F(TimerWheelTest, Simple) {
  int check = 0;
  // 0ms
  tw_.AddTimer(0, [&check] {
    check--;
  });
  check++;
  tw_.MoveOn();
  ASSERT_EQ(0, check);
  // 5ms
  tw_.AddTimer(5, [&check] {
    check -= 5;
  });
  check += 5;
  usleep(2000);
  tw_.MoveOn();
  EXPECT_EQ(5, check);
  usleep(4000);
  tw_.MoveOn();
  ASSERT_EQ(0, check);
  // 500ms
  tw_.AddTimer(500, [&check] {
    check -= 500;
  });
  check += 500;
  usleep(501000);
  tw_.MoveOn();
  ASSERT_EQ(0, check);
}

TEST_F(TimerWheelTest, Owner) {
  int check = 0;
  {
    ccb::TimerOwner to;
    tw_.AddTimer(1, [&check] {
      check++;
    }, &to);
    EXPECT_EQ(1UL, tw_.GetTimerCount());
  }
  EXPECT_EQ(0UL, tw_.GetTimerCount());
  tw_.MoveOn();
  usleep(2000);
  tw_.MoveOn();
  EXPECT_EQ(0, check);

  {
    ccb::TimerOwner to;
    tw_.AddTimer(1, [&check] {
      check++;
    }, &to);
    EXPECT_EQ(1UL, tw_.GetTimerCount());
    usleep(2000);
    tw_.MoveOn();
    EXPECT_EQ(0UL, tw_.GetTimerCount());
    EXPECT_EQ(1, check--);
  }
  EXPECT_EQ(0UL, tw_.GetTimerCount());
  EXPECT_EQ(0, check);

  {
    ccb::TimerOwner to;
    tw_.AddTimer(1, [&check] {
      check++;
    }, &to);
    EXPECT_EQ(1UL, tw_.GetTimerCount());
    tw_.AddTimer(1, [&check] {
      check++;
    }, &to);
    EXPECT_EQ(1UL, tw_.GetTimerCount());
    usleep(2000);
    tw_.MoveOn();
    EXPECT_EQ(0UL, tw_.GetTimerCount());
    EXPECT_EQ(1, check--);
    tw_.AddTimer(1, [&check] {
      check++;
    }, &to);
    EXPECT_EQ(1UL, tw_.GetTimerCount());
  }
  EXPECT_EQ(0UL, tw_.GetTimerCount());
  usleep(2000);
  tw_.MoveOn();
  EXPECT_EQ(0, check);
}

TEST_F(TimerWheelTest, Cancel) {
  int check = 0;
  ccb::TimerOwner to;
  tw_.AddTimer(1, [&check] {
    check++;
  }, &to);
  EXPECT_EQ(1UL, tw_.GetTimerCount());
  to.Cancel();
  EXPECT_EQ(0UL, tw_.GetTimerCount());
  usleep(2000);
  tw_.MoveOn();
  EXPECT_EQ(0, check);

  tw_.ResetTimer(to, 1);
  EXPECT_EQ(1UL, tw_.GetTimerCount());
  usleep(2000);
  tw_.MoveOn();
  EXPECT_EQ(1, check);
  EXPECT_EQ(0UL, tw_.GetTimerCount());
  to.Cancel();
}

TEST_F(TimerWheelTest, Reset) {
  int check = 0;
  ccb::TimerOwner to;
  tw_.AddTimer(0, [&check] {
    check--;
  }, &to);
  // reset pending timer
  tw_.ResetTimer(to, 5);
  tw_.MoveOn();
  ASSERT_EQ(0, check);
  check++;
  usleep(10000);
  tw_.MoveOn();
  ASSERT_EQ(0, check);
  usleep(10000);
  // reset launched timer
  tw_.ResetTimer(to, 0);
  check++;
  tw_.MoveOn();
  ASSERT_EQ(0, check);
  // reset timer to period timer
  tw_.ResetTimer(to, 5);
  tw_.ResetPeriodTimer(to, 10);
  check += 2;
  usleep(25000);
  tw_.MoveOn();
  ASSERT_EQ(0, check);
}

static inline int64_t ts_diff(const timespec& ts1, const timespec& ts2) {
  return ((ts1.tv_sec - ts2.tv_sec) * 1000000000 + ts1.tv_nsec - ts2.tv_nsec);
}

TEST_F(TimerWheelTest, Period) {
  int check = 0;
  ASSERT_FALSE(tw_.AddPeriodTimer(0, []{}));
  ASSERT_TRUE(tw_.AddPeriodTimer(10, [&check] {
    check++;
  }));
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  for (int i = 1; i < 10; i++) {
    while (true) {
      usleep(500);
      tw_.MoveOn();
      struct timespec cts;
      clock_gettime(CLOCK_MONOTONIC, &cts);
      if (ts_diff(cts, ts) >= i*10*1000000)
        break;
    }
    EXPECT_EQ(i, check);
  }
}

TEST_F(TimerWheelTest, GetCurrentTick) {
  ccb::tick_t init_tick = tw_.GetCurrentTick();
  ccb::tick_t last_tick = init_tick;
  for (int i = 1; i < 10; i++) {
    usleep(500);
    tw_.MoveOn();
    ccb::tick_t cur_tick = tw_.GetCurrentTick();
    ASSERT_TRUE(last_tick == cur_tick || last_tick + 1 == cur_tick);
    last_tick = cur_tick;
  }
  ASSERT_LT(init_tick, last_tick);
}

PERF_TEST_F(TimerWheelTest, AddTimerPerf) {
  timers_++;
  tw_.AddTimer(1, [this]{
    timers_--;
  });
  if ((count_ & 0x3ff) == 0) {
    tw_.MoveOn();
  }
  if ((++count_ & 0x3fffff) == 1) {
    fprintf(stderr, "pending %d timers\n", timers_);
  }
}

PERF_TEST_F(TimerWheelTest, ResetTimerPerf) {
  static ccb::TimerOwner owner;
  if (!owner.has_timer()) {
    tw_.AddTimer(1, []{}, &owner);
  }
  tw_.ResetTimer(owner, 1);
}

class TimerWheelNoLockTest : public testing::Test
{
protected:
  TimerWheelNoLockTest() {
  }
  void SetUp() {
  }
  void TearDown() {
  }
  ccb::TimerWheel tw_{1000, false};
  int timers_ = 0;
  size_t count_ = 0;
};

PERF_TEST_F(TimerWheelNoLockTest, AddTimerPerf) {
  timers_++;
  tw_.AddTimer(1, [this]{
    timers_--;
  });
  if ((count_ & 0x3ff) == 0) {
    tw_.MoveOn();
  }
  if ((++count_ & 0x3fffff) == 1) {
    fprintf(stderr, "pending %d timers\n", timers_);
  }
}

PERF_TEST_F(TimerWheelNoLockTest, ResetTimerPerf) {
  static ccb::TimerOwner owner;
  if (!owner.has_timer()) {
    tw_.AddTimer(1, []{}, &owner);
  }
  tw_.ResetTimer(owner, 1);
}

class TimerWheelMTTest : public testing::Test
{
protected:
  void SetUp() {
    thread_ = std::thread([this] {
      while (!stop_) {
        tw_.MoveOn();
        usleep(200);
      }
    });
  }
  void TearDown() {
    stop_ = true;
    thread_.join();
  }
  ccb::TimerWheel tw_;
  std::thread thread_;
  size_t count_ = 0;
  std::atomic<int> timers_ = {0};
  std::atomic<bool> stop_ = {false};
};

TEST_F(TimerWheelMTTest, Simple) {
  std::atomic<int> check{0};
  // 0ms
  tw_.AddTimer(0, [&check] {
    check--;
  });
  check++;
  usleep(1000);
  ASSERT_EQ(0, check);
  // 5ms
  tw_.AddTimer(5, [&check] {
    check -= 5;
  });
  check += 5;
  usleep(2000);
  EXPECT_EQ(5, check);
  usleep(4000);
  ASSERT_EQ(0, check);
  // 500ms
  tw_.AddTimer(500, [&check] {
    check -= 500;
  });
  check += 500;
  usleep(501000);
  ASSERT_EQ(0, check);
}

PERF_TEST_F(TimerWheelMTTest, AddTimerPerf) {
  timers_++;
  tw_.AddTimer(1, [this]{
    timers_--;
  });
  if ((++count_ & 0x3fffff) == 1) {
    fprintf(stderr, "pending %d timers\n", static_cast<int>(timers_));
  }
}

