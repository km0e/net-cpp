/**
 * @file executor.cpp
 * @brief Test for ThreadPoolExecutor
 */
#include <gtest/gtest.h>
#include <xsl/coro.h>

#include <atomic>
#include <chrono>
#include <latch>
#include <semaphore>
#include <thread>
#include <vector>

using namespace xsl;

TEST(ThreadPoolExecutor, SingleTask) {
  ThreadPoolExecutor pool{4};
  std::atomic<int> result = 0;
  std::binary_semaphore done{0};

  pool.schedule([&] {
    result.store(42);
    done.release();
  });

  done.acquire();
  EXPECT_EQ(result.load(), 42);
}

TEST(ThreadPoolExecutor, MultipleTasks) {
  ThreadPoolExecutor pool{4};
  constexpr int N = 1000;
  std::atomic<int> counter = 0;
  std::binary_semaphore done{0};

  for (int i = 0; i < N; i++) {
    pool.schedule([&] {
      if (counter.fetch_add(1) == N - 1) done.release();
    });
  }

  done.acquire();
  EXPECT_EQ(counter.load(), N);
}

TEST(ThreadPoolExecutor, RunsOnWorkerThread) {
  ThreadPoolExecutor pool{4};
  std::thread::id main_tid = std::this_thread::get_id();
  std::atomic<bool> on_worker = false;
  std::binary_semaphore done{0};

  pool.schedule([&] {
    on_worker.store(std::this_thread::get_id() != main_tid);
    done.release();
  });

  done.acquire();
  EXPECT_TRUE(on_worker.load());
}

TEST(ThreadPoolExecutor, UsesAllWorkers) {
  constexpr size_t N = 4;
  ThreadPoolExecutor pool{N};
  std::atomic<int> max_concurrent = 0;
  std::atomic<int> current = 0;
  std::latch start{1};
  std::binary_semaphore done{0};

  for (size_t i = 0; i < N; i++) {
    pool.schedule([&] {
      start.wait();  // gate: all workers wait
      int c = current.fetch_add(1) + 1;
      int prev = max_concurrent.load();
      while (c > prev && !max_concurrent.compare_exchange_weak(prev, c))
        ;
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      current.fetch_sub(1);
    });
  }
  pool.schedule([&] {
    start.wait();
    done.release();
  });

  start.count_down();
  done.acquire();
  EXPECT_GE(max_concurrent.load(), 2);
}

TEST(ThreadPoolExecutor, ConcurrentSubmits) {
  ThreadPoolExecutor pool{4};
  constexpr int N = 1000;
  std::atomic<int> counter = 0;
  std::binary_semaphore done{0};

  auto producer = [&] {
    for (int i = 0; i < N / 4; i++) {
      pool.schedule([&] {
        if (counter.fetch_add(1) == N - 1) done.release();
      });
    }
  };

  std::jthread t1(producer);
  std::jthread t2(producer);
  std::jthread t3(producer);
  std::jthread t4(producer);
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  done.acquire();
  EXPECT_EQ(counter.load(), N);
}

TEST(ThreadPoolExecutor, DrainBeforeDestroy) {
  std::atomic<int> counter = 0;
  std::binary_semaphore done{0};

  {
    ThreadPoolExecutor pool{2};
    for (int i = 0; i < 100; i++) {
      pool.schedule([&] {
        if (counter.fetch_add(1) == 99) done.release();
      });
    }
    done.acquire();
    EXPECT_EQ(counter.load(), 100);
  }
  EXPECT_EQ(counter.load(), 100);
}
