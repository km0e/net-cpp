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
  Signal<1> sig{};
  auto pin = sig.pin();
  auto moved = std::move(sig);
  ASSERT_TRUE(pin);
  pin.release();
  ASSERT_TRUE([&pin] -> Task<bool> { co_return co_await pin; }().block());
}

TEST(Signal, SafeStop) {
  const int N = 100000;
  Signal<N> sig{};
  int count = 0;
  std::jthread consumer([&] {
    while ([&] -> Task<bool> { co_return co_await sig; }().block()) {
      ++count;
    }
  });
  std::jthread producer([&] {
    for (int i = 0; i < N; i++) {
      sig.release();
    }
  });
  producer.join();
  sig.stop();
  consumer.join();
  ASSERT_EQ(count, N);
};

TEST(Signal, SafeForceStop) {
  const int N = 100000;
  Signal<N> sig{};
  int count = 0;
  std::jthread consumer([&] {
    while ([&] -> Task<bool> { co_return co_await sig; }().block()) {
      ++count;
    }
  });
  std::jthread producer([&] {
    for (int i = 0; i < N; i++) {
      sig.release();
    }
  });
  producer.join();
  volatile int rest = sig.stop<true>();  // TODO: bug?
  consumer.join();
  INFO("res: {}, count: {}", (int)rest, count);
  ASSERT_EQ(rest + count, N);
};

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
