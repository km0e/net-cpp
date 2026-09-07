/**
 * @file cancellable.cpp
 * @brief Tests for MPSCSignal and Cancellable<MPSCSignal>
 */
#include <gtest/gtest.h>
#include <xsl/coro.h>

#include <semaphore>
#include <thread>

using namespace xsl;

// --- Plain MPSCSignal ---

TEST(MPSCSignal, ReleaseBeforeAwait) {
  MPSCSignal sig;
  sig.release();
  EXPECT_TRUE(sig.await_ready());
  EXPECT_TRUE(sig.await_resume());
}

TEST(MPSCSignal, ReleaseBeforeConsumer) {
  MPSCSignal sig;
  sig.release();  // release before consumer exists

  int count = 0;
  [](auto& sig, int& count) -> Task<void> {
    if (co_await sig) count++;
  }(sig, count)
                                   .detach(coro::CoroContext{});

  ASSERT_EQ(count, 1);  // signal was already pending
}

// --- Cancellable<MPSCSignal> ---

TEST(CancellableMPSC, CancelWakesConsumer) {
  auto ctx = CoroContext(NewThreadExecutor{});
  MPSCSignal sig;
  std::atomic<int> count{0};
  std::binary_semaphore ready{0}, consumed{0}, done{0};

  [](auto& sig, auto& count, auto& ready, auto& consumed, auto& done) -> Task<void> {
    ready.release();
    auto csig = cancellable(sig);
    while (co_await csig) {
      count.fetch_add(1);
      consumed.release();
    }
    done.release();
  }(sig, count, ready, consumed, done)
                                                            .by(ctx)
                                                            .detach(ctx);

  ready.acquire();  // consumer suspended in first await
  sig.release();
  consumed.acquire();  // handshake: the release was consumed
  ASSERT_EQ(count.load(), 1);

  // Cancel — should wake consumer and exit while loop
  ctx.cancel();
  done.acquire();
  ASSERT_EQ(count.load(), 1);  // no more signals consumed
}

TEST(CancellableMPSC, CancelBeforeAwait) {
  auto ctx = CoroContext(NewThreadExecutor{});
  MPSCSignal sig;
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

TEST(CancellableMPSC, RepeatedAwaitsDoNotLeakRegistrations) {
  // regression for A3: each normal (non-cancelled) await must unregister its
  // wakeup continuation; after the loop the ctx continuation slot must be null
  auto ctx = CoroContext(NewThreadExecutor{});
  MPSCSignal sig;
  const int N = 50;
  std::atomic<int> count{0};
  std::binary_semaphore consumed{0}, done{0};

  [](auto& sig, int n, auto& count, auto& consumed, auto& done) -> Task<void> {
    auto csig = cancellable(sig);
    for (int i = 0; i < n; i++) {
      if (!co_await csig) break;
      count.fetch_add(1);
      consumed.release();
    }
    done.release();
  }(sig, N, count, consumed, done)
                                                       .by(ctx)
                                                       .detach(ctx);

  for (int i = 0; i < N; i++) {
    sig.release();
    consumed.acquire();  // handshake: every release consumed exactly once
  }
  done.acquire();
  ASSERT_EQ(count.load(), N);
  EXPECT_EQ(ctx.cc()->load(std::memory_order_acquire), nullptr)
      << "wakeup continuation leaked after normal completion";
}
