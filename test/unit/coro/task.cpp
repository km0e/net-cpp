/**
 * @file test_task.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "coro/tool.h"

#include <gtest/gtest.h>
#include <xsl/coro/core/executor.h>
#include <xsl/error.h>

using namespace xsl::coro;

TEST(Task, just_return) {
  int value = 0;
  no_return_task(value).block();
  ASSERT_EQ(value, 1);
  EXPECT_EQ(return_task().block(), 1);

  auto executor = std::make_shared<NewThreadExecutor>();

  std::binary_semaphore sem{0};
  sync_no_return_task(value, sem).detach(executor);
  sem.acquire();
  EXPECT_EQ(value, 2);
}

TEST(Task, just_yield) {
  int value = 0;
  [&value]() -> Task<void> {
    co_yield no_return_task(value);
    co_return;
  }()
                    .block();
  ASSERT_EQ(value, 1);
}

TEST(Task, just_throw) {
  ASSERT_THROW(no_return_exception_task().block(), std::runtime_error);

  ASSERT_THROW(return_exception_task().block(), std::runtime_error);
}

TEST(Task, multi_task) {
  int value = 0;
  multi_task(value).block();
  ASSERT_EQ(value, 2);

  auto executor = std::make_shared<NewThreadExecutor>();

  std::binary_semaphore sem{0};
  sync_multi_task(value, sem).detach(executor);
  sem.acquire();
  EXPECT_EQ(value, 4);
}

TEST(Task, penetrate_exception) {
  ASSERT_THROW(exception_penetrate_task().block(), std::runtime_error);
}

Task<Expected<int, int>> func1() { co_return {1}; }

TEST(Task, and_then) {
  EXPECT_EQ(*func1().and_then([](auto) { return Expected<int, int>{2}; }).block(), 2);
}

TEST(Task, map) {
  EXPECT_EQ(*func1().map([](auto v) { return v + 1; }).block(), 2);
  EXPECT_EQ(*func1()
                 .and_then([](auto) { return Expected<int, int>{2}; })
                 .map([](auto v) { return v + 1; })
                 .block(),
            3);
  EXPECT_EQ(*func1().map([](auto v) { return v + 1; }).map([](auto v) { return v + 1; }).block(),
            3);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
