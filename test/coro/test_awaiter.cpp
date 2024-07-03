#include "xsl/coro/await.h"
#include "xsl/coro/task.h"
#include "xsl/logctl.h"

#include <gtest/gtest.h>

#include <thread>
using namespace xsl::coro;

TEST(CallbackAwaiterTest, CallbackAwaiter) {
  auto task
      = []() -> Task<int> { co_return co_await CallbackAwaiter<int>([](auto cb) { cb(42); }); };
  ASSERT_EQ(task().block(), 42);
  auto task2 = []() -> Task<int> {
    co_return co_await CallbackAwaiter<int>([](auto cb) {
      std::thread([cb]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        cb(42);
      }).detach();
    });
  };
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  ASSERT_EQ(task2().block(), 42);
}

int main(int argc, char **argv) {
  xsl::no_log();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
