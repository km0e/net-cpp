/**
 * @file guard.cpp
 * @brief Test for ArgGuard — preserves arguments across suspend points
 *
 * ArgGuard defers awaiter creation until operator co_await(), keeping the
 * constructor arguments alive in a unique_ptr<tuple>. This prevents
 * use-after-free when an awaiter depends on temporary arguments.
 *
 * Known limitation: operator co_await() is &&-qualified and Task's
 * await_transform intercepts co_await before operator co_await() runs.
 * Tests below use operator co_await() directly and .block() the result.
 */
#include <gtest/gtest.h>
#include <xsl/coro.h>

using namespace xsl;

// Basic: args preserved and correctly forwarded through deferred factory
TEST(ArgGuard, PreservesValueArguments) {
  auto guard = ArgGuard(
      [](int x) -> Task<int> { co_return x * 2; },
      21
  );

  // operator co_await() applies the factory to preserved args, producing a
  // Task<int>. ArgGuardAwaiter<Task<int>> inherits Task<int> so .block() works.
  auto result = std::move(guard).operator co_await().block();
  EXPECT_EQ(result, 42);
}

// Key advantage: keeps temporaries alive past the constructor call.
// Without ArgGuard, std::string("hello world") would be destroyed before
// the factory executes. ArgGuard's unique_ptr<tuple> extends its lifetime.
TEST(ArgGuard, KeepsTemporaryAlive) {
  auto guard = ArgGuard(
      [](const std::string& s) -> Task<int> { co_return static_cast<int>(s.size()); },
      std::string("hello world")
  );
  auto result = std::move(guard).operator co_await().block();
  EXPECT_EQ(result, 11);
}

// Multiple arguments preserved in the tuple
TEST(ArgGuard, MultipleArguments) {
  auto guard = ArgGuard(
      [](int a, int b, int c) -> Task<int> { co_return a + b + c; },
      1, 2, 3
  );
  auto result = std::move(guard).operator co_await().block();
  EXPECT_EQ(result, 6);
}

// Factory is called at operator co_await() time, not at construction.
// Deferred invocation means no work is done until the co_await point.
TEST(ArgGuard, FactoryCalledAtCoAwait) {
  bool invoked = false;

  auto guard = ArgGuard(
      [&invoked](int x) -> Task<int> {
        invoked = true;
        co_return x;
      },
      42
  );

  EXPECT_FALSE(invoked);  // not called at construction

  auto result = std::move(guard).operator co_await().block();

  EXPECT_TRUE(invoked);   // called during operator co_await()
  EXPECT_EQ(result, 42);
}

