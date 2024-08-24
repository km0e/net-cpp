#include "xsl/coro.h"

#include <gtest/gtest.h>

#include <cassert>
#include <thread>
using namespace xsl;

TEST(SemaphoreTest, Basic) {
  CountingSemaphore<1> sem{};
  int value = 0;
  int res = 0;
  std::thread t1([&] {
    [&]() -> Task<void> {
      if (co_await sem) {
        res = value;
      }
      co_return;
    }()
                 .block();
  });
  std::thread t2([&] {
    value = 1;
    sem.release();
  });
  t1.join();
  t2.join();
  ASSERT_EQ(res, 1);
};
TEST(SemaphoreTest, State) {
  CountingSemaphore<1> sem{};
  int value = 0;
  int res = 0;
  std::thread t1([&] {
    [&]() -> Task<void> {
      if (!co_await sem) res = value;
      co_return;
    }()
                 .block();
  });
  std::thread t2([&] {
    value = 1;
    sem.release(false);
  });
  t1.join();
  t2.join();
  ASSERT_EQ(res, 1);
};

int main(int argc, char **argv) {
  xsl::no_log();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
