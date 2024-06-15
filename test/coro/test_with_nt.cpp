#include "tool.h"
#include "xsl/coro/executor.h"
#include "xsl/coro/task.h"

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <thread>
using namespace xsl::coro;

TEST(Task, just_return) {
  int value = 0;
  auto executor = std::make_shared<NewThreadExecutor>();

  no_return_task<NewThreadExecutor>(value).by(executor).block();
  ASSERT_EQ(value, 1);

  no_return_task<NewThreadExecutor>(value).by(executor).detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_EQ(value, 1);

  ASSERT_EQ(return_task<NewThreadExecutor>().by(executor).block(), 2);
}

TEST(Task, just_throw) {
  auto executor = std::make_shared<NewThreadExecutor>();
  ASSERT_THROW(no_return_exception_task<NewThreadExecutor>().by(executor).block(),
               std::runtime_error);

  ASSERT_THROW(return_exception_task<NewThreadExecutor>().by(executor).block(), std::runtime_error);

}

TEST(Task, async_task) {
  auto executor = std::make_shared<NewThreadExecutor>();
  int value = 0;
  auto task1 = [](int &value) -> Task<void, NewThreadExecutor> {
    co_await no_return_task<NewThreadExecutor>(value);
    value += 1;
    co_return;
  };
  task1(value).by(executor).block();
  ASSERT_EQ(value, 2);

  task1(value).by(executor).detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_EQ(value, 2);

  auto task2 = [](int &value) -> Task<void, NewThreadExecutor> {
    value = co_await return_task() + 1;
    co_return;
  };
  task2(value).by(executor).block();
  ASSERT_EQ(value, 3);

  task2(value).by(executor).detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_EQ(value, 3);

  auto task3 = []() -> Task<int, NewThreadExecutor> {
    int value = 0;
    co_await no_return_task(value);
    co_return value + 1;
  };
  ASSERT_EQ(task3().by(executor).block(), 2);

  auto task4 = []() -> Task<int, NewThreadExecutor> { co_return co_await return_task() + 1; };
  ASSERT_EQ(task4().by(executor).block(), 3);
}

TEST(Task, async_exception_task) {
  auto executor = std::make_shared<NoopExecutor>();
  ASSERT_THROW(
      []() -> Task<void> {
        co_await no_return_exception_task();
        co_return;
      }()
                  .by(executor)
                  .block(),
      std::runtime_error);

  ASSERT_THROW(
      []() -> Task<int> { co_return co_await return_exception_task() + 1; }().by(executor).block(),
      std::runtime_error);

  ASSERT_THROW(
      []() -> Task<int> {
        co_await no_return_exception_task();
        co_return 1;
      }()
                  .by(executor)
                  .block(),
      std::runtime_error);

  ASSERT_THROW(
      []() -> Task<int> { co_return co_await return_exception_task() + 1; }().by(executor).block(),
      std::runtime_error);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  spdlog::set_level(spdlog::level::trace);
  return RUN_ALL_TESTS();
}
