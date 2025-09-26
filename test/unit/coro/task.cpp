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

  std::binary_semaphore sem{0};
  sync_no_return_task(value, sem).detach(CoroContext(NewThreadExecutor()));
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

  std::binary_semaphore sem{0};
  sync_multi_task(value, sem).detach(NewThreadExecutor());
  sem.acquire();
  EXPECT_EQ(value, 4);
}

TEST(Task, penetrate_exception) {
  ASSERT_THROW(exception_penetrate_task().block(), std::runtime_error);
}

Task<Expected<int, int>> func1() { co_return {1}; }
Task<Expected<void, int>> func2() { co_return {}; }

TEST(Task, and_then) {
  EXPECT_EQ(*func1().and_then([](auto) { return Expected<int, int>{2}; }).block(), 2);
}

Task<int> then_chain_task() { co_return 1; }

TEST(Task, then_chain) {
  EXPECT_EQ(then_chain_task().then([](int x) { return x + 1; }).block(), 2);
  EXPECT_EQ(
      then_chain_task().then([](int x) { return x * 2; }).then([](int x) { return x + 3; }).block(),
      5);
}

Task<int> then_chain_5_task() { co_return 1; }

TEST(Task, then_chain_5) {
  EXPECT_EQ(then_chain_5_task()
                .then([](int x) { return x + 1; })
                .then([](int x) { return x * 2; })
                .then([](int x) { return x + 3; })
                .then([](int x) { return x * 5; })
                .then([](int x) { return x - 1; })
                .block(),
            34);
}

Task<int> then_chain_10_task() { co_return 0; }

TEST(Task, then_chain_10) {
  // 10-level chain compiles (validates O(1) instantiation, no template depth explosion)
  auto t = then_chain_10_task()
               .then([](int x) { return x + 1; })
               .then([](int x) { return x + 1; })
               .then([](int x) { return x + 1; })
               .then([](int x) { return x + 1; })
               .then([](int x) { return x + 1; })
               .then([](int x) { return x + 1; })
               .then([](int x) { return x + 1; })
               .then([](int x) { return x + 1; })
               .then([](int x) { return x + 1; })
               .then([](int x) { return x + 1; });
  static_assert(std::is_same_v<decltype(t)::result_type, int>);
}

TEST(Task, then_move_only) {
  using T = decltype(then_chain_task().then([](int x) { return std::make_unique<int>(x + 42); }));
  static_assert(std::is_same_v<T::result_type, std::unique_ptr<int>>);
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
  EXPECT_EQ(func2().map([] {}).block(), (std::expected<void, int>{}));
  EXPECT_EQ(func2().map([] {}).map([] {}).block(), (std::expected<void, int>{}));
  EXPECT_EQ(func2().map([] { return 1; }).block(), (std::expected<int, int>{1}));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
