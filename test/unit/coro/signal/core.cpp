/**
 * @file core.cpp
 * @brief Semantics tests for the unified signal core (UnsafeSignal / MPSCSignal):
 *        release/await roundtrip, sticky stop, retraction on racy suspend
 */
#include <gtest/gtest.h>
#include <xsl/coro.h>

#include <semaphore>
#include <thread>

using namespace xsl;

namespace {
  struct ProbePromise {
    std::atomic<bool> resumed{false};
    template <class P>
    void resume(std::coroutine_handle<P>) {
      resumed.store(true, std::memory_order_release);
    }
  };
  struct ProbeCoro {
    struct promise_type : ProbePromise {
      ProbeCoro get_return_object() {
        return {std::coroutine_handle<promise_type>::from_promise(*this)};
      }
      std::suspend_always initial_suspend() { return {}; }
      std::suspend_always final_suspend() noexcept { return {}; }
      void return_void() {}
      void unhandled_exception() {}
    };
    std::coroutine_handle<promise_type> h;
    ~ProbeCoro() { h.destroy(); }
  };
  ProbeCoro probe_coro() { co_return; }
}  // namespace

// --- both tiers: release before await ---

TEST(SignalCore, ReleaseBeforeAwait) {
  {
    UnsafeSignal u;
    u.release();
    EXPECT_TRUE(u.await_ready());
    EXPECT_TRUE(u.await_resume());
  }
  {
    MPSCSignal sig;
    sig.release();
    EXPECT_TRUE(sig.await_ready());
    EXPECT_TRUE(sig.await_resume());
  }
}

// --- both tiers: await before release, woken via coroutine ---

template <class Sig>
void await_before_release(Sig& sig) {
  int count = 0;
  [](auto& sig, int& count) -> Task<void> {
    if (co_await sig) count++;
  }(sig, count)
                                   .detach(coro::CoroContext{});
  sig.release();
  ASSERT_EQ(count, 1);
}

TEST(SignalCore, AwaitBeforeRelease) {
  UnsafeSignal us;
  await_before_release(us);
  MPSCSignal as;
  await_before_release(as);
}

// --- sticky stop: await AFTER stop must not hang ---

TEST(SignalCore, AwaitAfterStopDoesNotSuspend) {
  MPSCSignal sig;
  sig.stop();
  EXPECT_TRUE(sig.await_ready());    // stop is observed as "ready"
  EXPECT_FALSE(sig.await_resume());  // ... and reported as stopped

  UnsafeSignal us;
  us.stop();
  EXPECT_TRUE(us.await_ready());
  EXPECT_FALSE(us.await_resume());
}

// --- stop is sticky: release() after stop() is ignored ---

TEST(SignalCore, ReleaseAfterStopIsIgnored) {
  MPSCSignal sig;
  sig.stop();
  sig.release();
  EXPECT_TRUE(sig.stopped());
  EXPECT_TRUE(sig.await_ready());
  EXPECT_FALSE(sig.await_resume());  // still stopped, not signaled

  UnsafeSignal us;
  us.stop();
  us.release();
  EXPECT_TRUE(us.stopped());
  EXPECT_FALSE(us.await_resume());
}

// --- stop() wakes a suspended consumer with false ---

TEST(SignalCore, StopWakesSuspendedConsumer) {
  MPSCSignal sig;
  int count = 0;
  std::binary_semaphore ready{0}, done{0};

  [](auto& sig, int& count, auto& ready, auto& done) -> Task<void> {
    ready.release();
    while (co_await sig) count++;
    done.release();
  }(sig, count, ready, done)
                                   .detach(coro::CoroContext(NewThreadExecutor{}));

  ready.acquire();
  sig.release();
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  sig.stop();
  done.acquire();
  ASSERT_EQ(count, 1);
  EXPECT_TRUE(sig.stopped());
}

// --- MPSC: multiple producers racing to release; every release is consumed.
//     A permit semaphore prevents coalescing so the consumer can count them;
//     producers still race each other (and the consumer's suspend window). ---

TEST(SignalCore, MultiProducerStress) {
  const int N = 10000;
  MPSCSignal sig;
  std::atomic<int> consumed{0};
  std::binary_semaphore permit{1};  // consumer re-arms after each consume
  std::jthread consumer([&] {
    for (int i = 0; i < N; i++) {
      if (!sig.await_ready()) {
        auto probe = probe_coro();  // RAII destroys the frame
        if (sig.await_suspend(probe.h)) {
          while (!probe.h.promise().resumed.load(std::memory_order_acquire)) {
            std::this_thread::yield();
          }
        }
      }
      if (sig.await_resume()) consumed.fetch_add(1);
      permit.release();
    }
  });
  auto produce = [&](int n) {
    for (int i = 0; i < n; i++) {
      permit.acquire();
      sig.release();
    }
  };
  {
    std::jthread p1([&] { produce(N / 2); });
    std::jthread p2([&] { produce(N / 2 + N % 2); });
  }
  consumer.join();
  EXPECT_EQ(consumed.load(), N);
}

// --- await_resume const-correctness / direct use through references ---

TEST(SignalCore, UsableThroughReference) {
  MPSCSignal sig;
  auto& ref = sig;
  ref.release();
  EXPECT_TRUE(ref.await_ready());
  EXPECT_TRUE(ref.await_resume());
}
