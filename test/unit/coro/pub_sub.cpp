/**
 * @file test_pub_sub.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Test for Publish-Subscribe pattern for coroutines
 * @version 0.21
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
TEST(ExactPubSub, Exit) {
  auto pubsub = _coro::make_pub_sub<_value_pack<1>, SPSCSignal2<1>, Shared>();
  pubsub->publish<1>();
  int count = 0;
  [](auto sub, int &count) -> Task<void> {
    while (co_await *sub->template signal<1>()) {
      count++;
    }
  }(std::move(pubsub), count)
                                  .detach();
  ASSERT_EQ(count, 1);
}

TEST(ExactPubSub, UnSafeExit) {
  auto pubsub = _coro::make_pub_sub<_value_pack<1>, UnsafeSignal<1>, Shared>();
  pubsub->publish<1>();
  int count = 0;
  [](auto sub, int &count) -> Task<void> {
    while (co_await *sub->template signal<1>()) {
      count++;
    }
  }(std::move(pubsub), count)
                                  .detach();
  ASSERT_EQ(count, 1);
}

TEST(ExactPubSub, PubByPred) {
  auto pubsub = _coro::make_pub_sub<_value_pack<1>, SPSCSignal2<1>, Shared>();
  pubsub->publish([](int v) { return v == 1; });
  int count = 0;
  [](auto sub, int &count) -> Task<void> {
    while (co_await *sub->template signal<1>()) {
      count++;
    }
  }(std::move(pubsub), count)
                                  .detach();
  ASSERT_EQ(count, 1);
}

TEST(PubSub, SafeExit) {
  auto pubsub = [] -> auto {
    auto pubsub = _coro::make_pub_sub<int, SPSCSignal2<100>>();
    pubsub.subscribe(1);
    pubsub.template publish<1>();
    return pubsub;
  }();
  int count = 0;
  [](auto sub, int &count) -> Task<void> {
    while (co_await *sub.template signal<1>()) {
      count++;
    }
  }(std::move(pubsub), count)
                                  .detach();
  ASSERT_EQ(count, 1);
}

TEST(PubSub, PubByPred) {
  auto pubsub = _coro::make_pub_sub<int, SPSCSignal2<100>>();
  auto [sig, _] = pubsub.subscribe(1);
  pubsub.subscribe(2);
  pubsub.publish([](const int &v) { return v == 1; });
  int count = 0;
  [](auto sub, int &count) -> Task<void> {
    while (co_await *sub) {
      count++;
    }
  }(std::move(sig), count)
                                  .detach();
  ASSERT_EQ(count, 1);
}

TEST(PubSub, HeavyConcurrent) {
  UniformDistributionGenerator gen{};
  auto executor = std::make_shared<coro::NewThreadExecutor>();
  auto rand_pub = gen.generate(100000, 1, 100);
  auto rand_sub = gen.generate(10, 1, 100);

  auto pubsub = _coro::make_pub_sub<int, SPSCSignal2<100000>>();

  std::unordered_map<int, int> counter{};
  for (auto i : rand_sub) {
    counter.try_emplace(i);
  }
  std::counting_semaphore<> sem{0};
  for (auto i : rand_sub) {
    [](int v, auto &pubsub, auto &sem, auto &counter) -> Task<void> {
      auto [sig, ok] = pubsub.subscribe(v);
      if (!ok) {
        co_return;
      }
      while (co_await *sig) {
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
