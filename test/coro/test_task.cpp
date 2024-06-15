#include "xsl/coro/task.h"
#include "tool.h"

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
using namespace xsl::coro;


TEST(Task, just_return) {
  int value = 0;

  no_return_task(value).block();
  ASSERT_EQ(value, 1);

  ASSERT_EQ(return_task().block(), 2);
}

TEST(Task, just_throw) {
  ASSERT_THROW(no_return_exception_task().block(), std::runtime_error);

  ASSERT_THROW(return_exception_task().block(), std::runtime_error);
}

TEST(Task, async_task) {
  int value = 0;
  auto task1 = [](int &value) -> Task<void> {
    co_await no_return_task(value);
    value += 1;
    co_return;
  }(value);
  task1.block();
  ASSERT_EQ(value, 2);

  auto task2 = [](int &value) -> Task<void> {
    value = co_await return_task() + 1;
    co_return;
  }(value);
  task2.block();
  ASSERT_EQ(value, 3);

  auto task3 = []() -> Task<int> {
    int value = 0;
    co_await no_return_task(value);
    co_return value + 1;
  }();
  ASSERT_EQ(task3.block(), 2);

  auto task4 = []() -> Task<int> {
    co_return co_await return_task() + 1;
  }();
  ASSERT_EQ(task4.block(), 3);
}

TEST(Task, async_exception_task) {
  ASSERT_THROW([]() -> Task<void> {
    co_await no_return_exception_task();
    co_return;
  }().block(), std::runtime_error);

  ASSERT_THROW([]() -> Task<int> {
    co_return co_await return_exception_task() + 1;
  }().block(), std::runtime_error);

  ASSERT_THROW([]() -> Task<int> {
    co_await no_return_exception_task();
    co_return 1;
  }().block(), std::runtime_error);

  ASSERT_THROW([]() -> Task<int> {
    co_return co_await return_exception_task() + 1;
  }().block(), std::runtime_error);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  spdlog::set_level(spdlog::level::trace);
  return RUN_ALL_TESTS();
}
