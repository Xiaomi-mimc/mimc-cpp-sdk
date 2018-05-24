#include <unistd.h>
#include <array>
#include <memory>
#include <thread>
#include "gtestx/gtestx.h"
#include "ccbase/double_buf.h"

class DoubleBuf : public testing::Test
{
protected:
  DoubleBuf() : stop_(true) {}
  void SetUp() {
    db_shptr_.Update(std::make_shared<long>(0));
    w_thread_ = std::thread([this] {
      int i = 0;
      while (!stop_) {
        // write db_array_
        auto& arr = db_array_.mutable_buf();
        for (auto& n : arr) n = i;
        db_array_.Commit();
        // write db_shptr_
        db_shptr_.Update(std::make_shared<long>(0));
        usleep(1000);
      }
    });
  }
  void TearDown() {
    stop_ = true;
    w_thread_.join();
  }
  ccb::DoubleBuf<int> db_int_;
  ccb::DoubleBuf<std::array<long, 4>> db_array_;
  ccb::DoubleBuf<std::shared_ptr<long>> db_shptr_;
  std::thread w_thread_;
  std::atomic<bool> stop_;
};

TEST_F(DoubleBuf, SimpleReadWrite) {
  ASSERT_EQ(0, db_int_.buf());
  db_int_.Update(1);
  ASSERT_EQ(1, db_int_.buf());
  db_int_.Commit();
  ASSERT_EQ(0, db_int_.buf());
  db_int_.mutable_buf() = 2;
  ASSERT_EQ(0, db_int_.buf());
  db_int_.Commit();
  ASSERT_EQ(2, db_int_.buf());
}

TEST_F(DoubleBuf, ArrayTest) {
  for (int i = 0; i < 20000000; i++) {
    auto check = db_array_.buf()[0];
    for (size_t j = 0; j < db_array_.buf().size(); j++) {
      ASSERT_EQ(check, db_array_.buf()[j]);
    }
  }
}

PERF_TEST_F(DoubleBuf, ShptrPerf) {
  std::shared_ptr<long> shptr = db_shptr_.buf();
  ASSERT_EQ(0, *shptr) << PERF_ABORT;
}

