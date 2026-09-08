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
Task<Expected<void, int>> func2_err() { co_return std::unexpected{42}; }

TEST(Task, and_then) {
  EXPECT_EQ(*func1().and_then([](auto) { return Expected<int, int>{2}; }).block(), 2);
}

Task<int> then_chain_task() { co_return 1; }

Task<void> void_task() { co_return; }

// C4/then: Task<void> chains — the base feeds the innermost transform with
// no argument, and later transforms chain on its result
TEST(Task, then_void) {
  EXPECT_EQ(void_task().then([] { return 1; }).block(), 1);
  EXPECT_EQ(void_task().then([] {}).then([] { return 2; }).block(), 2);
  int called = 0;
  void_task().then([&] { called++; }).block();
  EXPECT_EQ(called, 1);
  EXPECT_EQ(void_task().then([] {}).then([] {}).then([] { return 3; }).block(), 3);
}

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
  auto res = then_chain_10_task()
                 .then([](int x) { return x + 1; })
                 .then([](int x) { return x + 1; })
                 .then([](int x) { return x + 1; })
                 .then([](int x) { return x + 1; })
                 .then([](int x) { return x + 1; })
                 .then([](int x) { return x + 1; })
                 .then([](int x) { return x + 1; })
                 .then([](int x) { return x + 1; })
                 .then([](int x) { return x + 1; })
                 .then([](int x) { return x + 1; })
                 .block();
  static_assert(std::is_same_v<decltype(res), int>);
  EXPECT_EQ(res, 10);
}

TEST(Task, then_move_only) {
  using T = decltype(then_chain_task().then([](int x) { return std::make_unique<int>(x + 42); }));
  static_assert(std::is_same_v<T::result_type, std::unique_ptr<int>>);
  // move-only results run through the chain intact
  auto p = then_chain_task().then([](int x) { return std::make_unique<int>(x + 42); }).block();
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 43);
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

// B8: co_return an lvalue reference (no implicit-move fallback applies)
TEST(Task, return_lvalue_ref) {
  int g = 7;
  auto f = [&g]() -> Task<int> {
    int& r = g;
    co_return r;  // lvalue reference — copies, no compile error
  };
  EXPECT_EQ(f().block(), 7);
}

// C4: and_then on Expected<void, E> (no-argument continuation)
TEST(Task, and_then_void) {
  EXPECT_TRUE(func2().and_then([] { return Expected<void, int>{}; }).block().has_value());
  EXPECT_EQ(func2().and_then([]() -> Expected<int, int> { return 3; }).block().value_or(0), 3);
  // error path: untouched by the continuation
  EXPECT_EQ(func2_err().and_then([]() -> Expected<int, int> { return 3; }).block().error_or(0), 42);
}

// B3: an explicit .by(ctx) must survive being co_await-ed — the parent must
// not clobber it (cancellation domain is the observable)
TEST(Task, by_context_survives_await) {
  auto ctxA = CoroContext{};
  MPSCSignal sig;
  std::atomic<int> count{0};
  std::binary_semaphore done{0};

  auto child = [&]() -> Task<void> {
    while (co_await sig) count.fetch_add(1);
    done.release();
  };
  auto parent = [&]() -> Task<void> {
    co_await child().by(ctxA);  // child keeps ctxA even though parent has its own
    co_return;
  };

  ctxA.cancel();  // before: must reach the child despite it being awaited
  parent().block();
  done.acquire();
  EXPECT_EQ(count.load(), 0);
}

// C1: a detached task that throws must not crash; the exception is logged
TEST(Detach, exception_is_contained) {
  []() -> Task<void> {
    throw std::runtime_error("detach test exception");
    co_return;
  }()
      .detach(CoroContext{});  // NoopExecutor: exception surfaces inline, logged
  SUCCEED();                  // reaching here means the exception was contained
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
