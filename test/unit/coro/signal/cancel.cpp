/**
 * @file cancel.cpp
 * @brief Test for signal cancellation via Cancellable + ctx.cancel()
 */
#include <CLI/CLI.hpp>
#include <gtest/gtest.h>
#include <xsl/coro.h>

#include <semaphore>
#include <thread>

using namespace xsl;

std::size_t TEST_COUNT = 1;

class SignalCancelTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}

  template <class Signal>
  void cancel(Signal& sig) {
    const int N = 10;
    std::atomic<int> count = 0;
    auto ctx = CoroContext(NewThreadExecutor{});
    std::binary_semaphore ready{0}, done{0};
    auto task = [](auto& sig, auto& count, auto& ready, auto& done) -> Task<void> {
      ready.release();  // signal: consumer is suspended
      while (co_await sig) count.fetch_add(1);
      done.release();  // signal: consumer exited
    }(sig, count, ready, done);
    std::move(task).detach(ctx);

    ready.acquire();  // wait for consumer to enter first await

    // Producer releases N signals — consumer should consume them all
    for (int i = 0; i < N; i++) {
      sig.release();
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    ASSERT_EQ(count.load(), N);

    // Cancel: wakeup callback → _cs=Yes + sig->release() → consumer wakes,
    // await_resume sees _cs=Yes → returns false → while exits
    ctx.cancel();
    done.acquire();              // wait for consumer to exit while loop
    ASSERT_EQ(count.load(), N);  // no extra counts after cancel
  }
};

TEST_F(SignalCancelTest, MPSC) {
  std::size_t N = TEST_COUNT;
  while (N--) {
    MPSCSignal m_sig{};
    cancel(m_sig);
  }
}

// Multi-threaded stress test: producer releases signals while main thread
// cancels. Exercises the TOCTOU window between set_cc and _cs check.
TEST_F(SignalCancelTest, MPSC_CancelRace) {
  for (int iter = 0; iter < 10; iter++) {
    auto ctx = CoroContext(NewThreadExecutor{});
    MPSCSignal sig;
    std::atomic<int> count = 0;
    std::binary_semaphore done{0};
    auto task = [](auto& sig, auto& count, auto& done) -> Task<void> {
      while (co_await sig) count.fetch_add(1);
      done.release();
    }(sig, count, done);
    std::move(task).detach(ctx);

    std::jthread producer([&] {
      for (int i = 0; i < 10; i++) {
        sig.release();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
    });

    std::this_thread::sleep_for(std::chrono::microseconds(500));
    ctx.cancel();  // race with set_cc in await_suspend
    done.acquire();
    producer.join();
  }
}

// C2: cancel() before set_cc(wakeup) — the wakeup is stored then immediately
// retrieved by set_cc(nullptr); the return value was previously leaked.
// 1000 iterations to stress and verify no memory growth.
TEST_F(SignalCancelTest, CancelBeforeSetCc) {
  for (int iter = 0; iter < 1000; iter++) {
    auto ctx = CoroContext(NewThreadExecutor{});
    MPSCSignal sig;
    std::atomic<int> count = 0;
    std::binary_semaphore done{0};

    ctx.cancel();  // cancel BEFORE consumer enters await_suspend
    auto task = [](auto& sig, auto& count, auto& done) -> Task<void> {
      while (co_await sig) count.fetch_add(1);
      done.release();
    }(sig, count, done);
    std::move(task).detach(ctx);

    done.acquire();
    ASSERT_EQ(count.load(), 0);  // cancelled — never consumed
  }
}

// C3: Race cancel() with concurrent suspend — cancel fires while
// await_suspend is swapping _cc, exercising the _cs TOCTOU window.
TEST_F(SignalCancelTest, CancelRaceSetCc) {
  for (int iter = 0; iter < 500; iter++) {
    auto ctx = CoroContext(NewThreadExecutor{});
    MPSCSignal sig;
    std::atomic<int> count = 0;
    std::binary_semaphore started{0}, done{0};

    
    std::jthread consumer([&] {
      auto task = [](auto& sig, auto& count, auto& started, auto& done) -> Task<void> {
        started.release();
        while (co_await sig) count.fetch_add(1);
        done.release();
      }(sig, count, started, done);
      std::move(task).detach(ctx);
    });

    started.acquire();  // consumer entering await_suspend
    ctx.cancel();       // race with set_cc
    done.acquire();
    consumer.join();
    ASSERT_EQ(count.load(), 0);
  }
}

int main(int argc, char** argv) {
  CLI::App app{"Cancel test"};
  app.add_option("-c,--count", TEST_COUNT, "Test count");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
