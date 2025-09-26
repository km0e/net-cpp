/**
 * @file spscv1v2.cpp
 * @brief Basic tests for SPSCSignal (v1) and SPSCSignal2
 */
#include <gtest/gtest.h>
#include <xsl/coro.h>

using namespace xsl;

TEST(SPSCSignalV1, ReleaseBeforeAwait) {
  SPSCSignal<> sig;
  sig.release();
  EXPECT_TRUE(sig.await_ready());
  EXPECT_GT(sig.await_resume(), 0);
}

TEST(SPSCSignalV1, AwaitBeforeRelease) {
  SPSCSignal<> sig;
  int count = 0;

  [](auto& sig, int& count) -> Task<void> {
    if (co_await sig) count++;
  }(sig, count)
                                   .detach(coro::CoroContext{});

  sig.release();
  ASSERT_EQ(count, 1);
}

TEST(SPSCSignal2, ReleaseBeforeAwait) {
  SPSCSignal2<> sig;
  sig.release();
  EXPECT_TRUE(sig.await_ready());
  EXPECT_GT(sig.await_resume(), 0);
}

// H1: UnsafeSignal stop() after release — variant holds ptrdiff_t, stop is a no-op
// on the callback path, and should not throw bad_variant_access.
TEST(UnsafeSignal, ReleaseThenStop) {
  UnsafeSignal<> sig;
  sig.release();  // variant holds ptrdiff_t{1}

  int v = [](auto& sig) -> Task<int> {
    co_return static_cast<int>(co_await sig);  // await_ready sees ptrdiff_t>0 → true
  }(sig)
                               .block();
  EXPECT_EQ(v, 1);

  sig.stop();  // variant is ptrdiff_t, not function — no crash
}

// H2: Verify spsc4 release() state is cleared before callback invocation.
// Multiple iterations to confirm no use-after-free on self.state.
TEST(SPSCSignal4, ReleaseDoesNotUseAfterFree) {
  for (int i = 0; i < 100; i++) {
    SPSCSignal4 sig;
    int count = 0;
    [](SPSCSignal4& sig, int& count) -> Task<void> {
      if (co_await sig) count++;
    }(sig, count)
                                            .detach(coro::CoroContext{});
    sig.release();
    ASSERT_EQ(count, 1);
  }
}

TEST(SPSCSignal2, AwaitBeforeRelease) {
  SPSCSignal2<> sig;
  int count = 0;

  [](auto& sig, int& count) -> Task<void> {
    if (co_await sig) count++;
  }(sig, count)
                                   .detach(coro::CoroContext{});

  sig.release();
  ASSERT_EQ(count, 1);
}
