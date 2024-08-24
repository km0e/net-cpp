#include "coro/tool.h"
#include "xsl/coro.h"
#include "xsl/logctl.h"

#include <gtest/gtest.h>
using namespace xsl::coro;

TEST(Lazy, just_return) {
  int value = 0;

  no_return_task(value).block();
  ASSERT_EQ(value, 1);

  ASSERT_EQ(return_task().block(), 1);
}

TEST(Lazy, just_throw) {
  ASSERT_THROW(no_return_exception_task().block(), std::runtime_error);

  ASSERT_THROW(return_exception_task().block(), std::runtime_error);
}

TEST(Lazy, async_task) {
  int value = 0;
  auto task1 = [](int &value) -> Lazy<void> {
    co_await no_return_task(value);
    value += 1;
    co_return;
  }(value);
  task1.block();
  ASSERT_EQ(value, 2);

  auto task2 = [](int &value) -> Lazy<void> {
    value = co_await return_task() + 1;
    co_return;
  }(value);
  task2.block();
  ASSERT_EQ(value, 2);

  auto task3 = []() -> Lazy<int> {
    int value = 0;
    co_await no_return_task(value);
    co_return value + 1;
  }();
  ASSERT_EQ(task3.block(), 2);

  auto task4 = []() -> Lazy<int> { co_return co_await return_task() + 1; }();
  ASSERT_EQ(task4.block(), 2);
}

TEST(Lazy, async_exception_task) {
  auto task1 = []() -> Lazy<void> {
    co_await no_return_exception_task();
    co_return;
  }();
  ASSERT_THROW(task1.block(), std::runtime_error);

  auto task2 = []() -> Lazy<void> {
    co_await return_exception_task();
    co_return;
  }();
  ASSERT_THROW(task2.block(), std::runtime_error);

  auto task3 = []() -> Lazy<int> {
    co_await no_return_exception_task();
    co_return 1;
  }();
  ASSERT_THROW(task3.block(), std::runtime_error);

  auto task4 = []() -> Lazy<int> { co_return co_await return_exception_task() + 1; }();

  ASSERT_THROW(task4.block(), std::runtime_error);
}

int main(int argc, char **argv) {
  xsl::no_log();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
