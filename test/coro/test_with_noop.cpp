#include "coro/tool.h"
#include "xsl/coro/task.h"

#include <gtest/gtest.h>
using namespace xsl::coro;

TEST(Task, just_return) {
  int value = 0;
  auto executor = std::make_shared<NoopExecutor>();

  no_return_task(value).by(executor).block();
  EXPECT_EQ(value, 1);

  no_return_task(value).by(executor).detach();
  EXPECT_EQ(value, 2);

  EXPECT_EQ(return_task().by(executor).block(), 1);
}

TEST(Task, just_throw) {
  auto executor = std::make_shared<NoopExecutor>();
  EXPECT_THROW(no_return_exception_task().by(executor).block(), std::runtime_error);

  EXPECT_THROW(return_exception_task().by(executor).block(), std::runtime_error);
}

TEST(Task, async_task) {
  auto executor = std::make_shared<NoopExecutor>();
  int value = 0;
  auto task1 = [](int &value) -> Task<void> {
    co_await no_return_task(value);
    value += 1;
    co_return;
  };
  task1(value).by(executor).block();
  EXPECT_EQ(value, 2);

  task1(value).by(executor).detach();
  ASSERT_EQ(value, 4);

  auto task2 = [](int &value) -> Task<void> {
    value = co_await return_task() + 1;
    co_return;
  };
  task2(value).by(executor).block();
  EXPECT_EQ(value, 2);

  task2(value).by(executor).detach();
  ASSERT_EQ(value, 2);

  auto task3 = []() -> Task<int> {
    int value = 0;
    co_await no_return_task(value);
    co_return value + 1;
  };
  EXPECT_EQ(task3().by(executor).block(), 2);

  auto task4 = []() -> Task<int> { co_return co_await return_task() + 1; };
  EXPECT_EQ(task4().by(executor).block(), 2);
}

TEST(Task, async_exception_task) {
  auto executor = std::make_shared<NoopExecutor>();
  auto task1 = []() -> Task<void> {
    co_await no_return_exception_task();
    co_return;
  }();
  EXPECT_THROW(task1.by(executor).block(), std::runtime_error);

  auto task2 = []() -> Task<int> { co_return co_await return_exception_task() + 1; }();
  EXPECT_THROW(task2.by(executor).block(), std::runtime_error);

  auto task3 = []() -> Task<int> {
    co_await no_return_exception_task();
    co_return 1;
  }();
  EXPECT_THROW(task3.by(executor).block(), std::runtime_error);

  auto task4 = []() -> Task<int> { co_return co_await return_exception_task() + 1; }();
  EXPECT_THROW(task4.by(executor).block(), std::runtime_error);
}

int main(int argc, char **argv) {
  xsl::no_log();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
