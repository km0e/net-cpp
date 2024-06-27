#include "xsl/coro/await.h"
#include "xsl/coro/task.h"

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

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
  testing::InitGoogleTest(&argc, argv);
  // spdlog::set_level(spdlog::level::debug);
  spdlog::set_pattern("%D-%T|%-5l|%-8s|%!|%v");
  return RUN_ALL_TESTS();
}
