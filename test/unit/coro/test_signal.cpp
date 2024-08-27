/**
 * @file test_signal.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "random.h"
#include "xsl/coro.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <thread>
#include <vector>
using namespace xsl;

TEST(Signal, Unsafe) {
  UniformDistributionGenerator gen{};
  auto seq = gen.generate(100, 0, 5);
  UnsafeSignal<int> sig{};
  for (int i = 0; i < 100; i++) {
    int value = -1;
    [&] -> Task<void> { value = co_await sig; }().detach();
    sig.release(seq[i]);
    ASSERT_EQ(value, seq[i]);
  };
}

TEST(Signal, Safe) {
  const int N = 1000000;
  UniformDistributionGenerator gen{};
  auto seq = gen.generate(N, 0, std::numeric_limits<int>::max());
  Signal<int> sig{};
  std::vector<int> values{};
  values.reserve(N);
  std::atomic_flag flag{};
  std::jthread consumer([&] {
    while (!flag.test()) {
      values.push_back([&] -> Task<int> { co_return co_await sig; }().block());
    }
  });
  std::jthread producer([&] {
    for (int i = 0; i < N - 1; i++) {
      sig.release(seq[i]);
    }
    flag.test_and_set();
    sig.release(seq[N - 1]);
  });
  producer.join();
  consumer.join();
  std::size_t idx = 0;
  for (int i = 0; i < N && idx < values.size(); i++) {
    if (values[idx] == seq[i]) {
      idx++;
    }
  }
  ASSERT_EQ(idx, values.size());
};

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
