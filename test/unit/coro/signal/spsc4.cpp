/**
 * @file spsc4.cpp
 * @brief Tests for SPSCSignal4 and Cancellable<SPSCSignal4>
 */
#include <gtest/gtest.h>
#include <xsl/coro.h>

#include <semaphore>
#include <thread>

using namespace xsl;

// --- Plain SPSCSignal4 ---

TEST(SPSCSignal4, ReleaseBeforeAwait) {
  SPSCSignal4 sig;
  sig.release();
  EXPECT_TRUE(sig.await_ready());
  EXPECT_TRUE(sig.await_resume());
}

TEST(SPSCSignal4, ReleaseBeforeConsumer) {
  SPSCSignal4 sig;
  sig.release();  // release before consumer exists

  int count = 0;
  [](auto& sig, int& count) -> Task<void> {
    if (co_await sig) count++;
  }(sig, count)
                                   .detach(coro::CoroContext{});

  ASSERT_EQ(count, 1);  // signal was already pending
}

// --- Cancellable<SPSCSignal4> ---

TEST(CancellableSPSCSignal4, CancelWakesConsumer) {
  auto ctx = CoroContext(NewThreadExecutor{});
  SPSCSignal4 sig;
  int count = 0;
  std::binary_semaphore ready{0}, done{0};

  [](auto& sig, int& count, auto& ready, auto& done) -> Task<void> {
    ready.release();
    auto csig = cancellable(sig);
    while (co_await csig) count++;
    done.release();
  }(sig, count, ready, done)
                                                            .by(ctx)
                                                            .detach(ctx);

  ready.acquire();  // consumer suspended in first await
  sig.release();
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  ASSERT_EQ(count, 1);

  // Cancel — should wake consumer and exit while loop
  ctx.cancel();
  done.acquire();
  ASSERT_EQ(count, 1);  // no more signals consumed
}

TEST(CancellableSPSCSignal4, CancelBeforeAwait) {
  auto ctx = CoroContext(NewThreadExecutor{});
  SPSCSignal4 sig;
  int count = 0;
  std::binary_semaphore done{0};

  ctx.cancel();  // cancel before consumer starts

  [](auto& sig, int& count, auto& done) -> Task<void> {
    auto csig = cancellable(sig);
    if (co_await csig) count++;
    done.release();
  }(sig, count, done)
                                               .by(ctx)
                                               .detach(ctx);

  done.acquire();
  ASSERT_EQ(count, 0);  // cancelled — never consumed
}
