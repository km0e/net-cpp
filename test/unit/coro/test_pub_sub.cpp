/**
 * @file test_pub_sub.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Test for Publish-Subscribe pattern for coroutines
 * @version 0.2
 * @date 2024-08-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "random.h"
#include "xsl/coro.h"

#include <gtest/gtest.h>

#include <semaphore>
#include <unordered_map>

using namespace xsl;

TEST(PubSub, SafeExit) {
  SignalReceiver rx = [] -> auto {
    PubSub<int, 100> pubsub{};
    auto sub_res = pubsub.subscribe(1);
    pubsub.publish(1);
    return std::move(*sub_res);
  }();
  int count = 0;
  [](SignalReceiver<> sub, int &count) -> Task<void> {
    while (co_await sub) {
      count++;
    }
  }(std::move(rx), count)
                                              .detach();
  ASSERT_EQ(count, 1);
}

TEST(PubSub, PubByPred) {
  PubSub<int, 100> pubsub{};
  auto sub_res = pubsub.subscribe(1);
  pubsub.subscribe(2);
  pubsub.publish([](int v) { return v == 1; });
  SignalReceiver rx = std::move(*sub_res);
  int count = 0;
  [](SignalReceiver<> sub, int &count) -> Task<void> {
    while (co_await sub) {
      count++;
    }
  }(std::move(rx), count)
                                              .detach();
  ASSERT_EQ(count, 1);
}

TEST(PubSub, HeavyConcurrent) {
  UniformDistributionGenerator gen{};
  auto executor = std::make_shared<coro::NewThreadExecutor>();
  auto rand_pub = gen.generate(100000, 1, 100);
  auto rand_sub = gen.generate(10, 1, 100);
  PubSub<int, 100> pubsub{};

  std::unordered_map<int, int> counter{};
  for (auto i : rand_sub) {
    counter.try_emplace(i);
  }
  std::counting_semaphore<> sem{0};
  for (auto i : rand_sub) {
    [](int v, auto &pubsub, auto &sem, auto &counter) -> Task<void> {
      auto sub_res = pubsub.subscribe(v);
      if (!sub_res) {
        co_return;
      }
      SignalReceiver rx = std::move(*sub_res);
      while (co_await rx) {
        counter[v]++;
      }
      sem.release();
    }(i, pubsub, sem, counter)
                                                             .detach(executor);
  }
  int total = 0;
  for (auto i : rand_pub) {
    if (pubsub.publish(i)) {
      total++;
    }
  }
  pubsub.stop();
  for (auto i{0u}; i < counter.size(); i++) {
    sem.acquire();
  }
  int sum = 0;
  for (auto [_, v] : counter) {
    sum += v;
  }
  ASSERT_EQ(sum, total);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
