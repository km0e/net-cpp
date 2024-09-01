/**
 * @file test_signal.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Test for signal
 * @version 0.21
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/coro.h"

#include <gtest/gtest.h>

#include <cassert>
#include <thread>
using namespace xsl;

TEST(Signal, Pin) {
  auto [r, w] = signal<1>();
  auto pin_r = r.pin();
  auto pin_w = w.pin();
  auto moved_r = std::move(r);
  auto moved_w = std::move(w);
  ASSERT_TRUE(pin_r);
  ASSERT_TRUE(pin_w);
  pin_w.release();
  ASSERT_TRUE([&pin_r] -> Task<bool> { co_return co_await pin_r; }().block());
}

TEST(Signal, SafeStop) {
  const int N = 100000;
  auto [r, w] = signal<N>();
  int count = 0;
  std::jthread consumer([&] {
    while ([&] -> Task<bool> { co_return co_await r; }().block()) {
      ++count;
    }
  });
  std::jthread producer([&] {
    for (int i = 0; i < N; i++) {
      w.release();
    }
  });
  producer.join();
  w.stop();
  consumer.join();
  ASSERT_EQ(count, N);
};

TEST(Signal, SafeForceStop) {
  const int N = 100000;
  auto [r, w] = signal<N>();
  int count = 0;
  std::jthread consumer([&] {
    while ([&] -> Task<bool> { co_return co_await r; }().block()) {
      ++count;
    }
  });
  std::jthread producer([&] {
    for (int i = 0; i < N; i++) {
      w.release();
    }
  });
  producer.join();
  count += w.stop<true>();//TODO: bug?
  consumer.join();
  ASSERT_EQ(count, N);
};

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
