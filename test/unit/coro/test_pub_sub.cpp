/**
 * @file test_pub_sub.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Test for Publish-Subscribe pattern for coroutines
 * @version 0.1
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

TEST(PubSub, Safe) {
  UniformDistributionGenerator gen{};
  auto executor = std::make_shared<coro::NewThreadExecutor>();
  auto rand_pub = gen.generate(100000, 1, 100);
  auto rand_sub = gen.generate(10, 1, 100);
  PubSub<int> pubsub{};

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
      Subscriber<int> sub = *sub_res;
      while (co_await sub) {
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
  pubsub.clear();
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
