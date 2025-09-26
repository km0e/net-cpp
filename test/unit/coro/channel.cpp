/**
 * @file channel.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Test for c
 * @version 0.1.0
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <CLI/CLI.hpp>
#include <gtest/gtest.h>
#include <xsl/coro.h>
#include <xsl/log.h>

#include <cassert>
#include <cstddef>
#include <thread>
#include <type_traits>
using namespace xsl;

std::size_t TEST_COUNT = 1;
class ChannelBasicTest : public ::testing::Test {
protected:
  ChannelBasicTest() {}
//
  void SetUp() override {}
//
  void TearDown() override {}
//
  ~ChannelBasicTest() {}
//
  template <class Channel>
  void spsc(Channel &sig) {
    const int N = 100000;
    int count = 0;

    // Push all items first, then consume — single-threaded avoids
    // stack-use-after-scope in block()'s NoopExecutor cross-thread access.
    for (int i = 0; i < N; i++) {
      ASSERT_TRUE(sig.push(i % 2 ? 1 : 0));
    }
    for (int i = 0; i < N; i++) {
      count += [&] -> Task<int> { co_return co_await sig; }().block();
    }
    ASSERT_EQ(count, N / 2);
  }

  // Multi-threaded: producer and consumer on separate jthreads.
  // NoopExecutor ensures callback runs inline on producer thread — consumer
  // coroutine hops to producer thread via the callback chain.
  template <class Channel>
  void mt_spsc(Channel &sig) {
    const int N = 10000;
    int count = 0;

    std::jthread consumer([&] {
      [](Channel& c, int& count) -> Task<void> {
        for (int i = 0; i < N; i++) count += co_await c;
      }(sig, count).detach(CoroContext{});
    });
    std::jthread producer([&] {
      for (int i = 0; i < N; i++) {
        ASSERT_TRUE(sig.push(i % 2 ? 1 : 0));
      }
    });
    producer.join();
    consumer.join();
    ASSERT_EQ(count, N / 2);
  }
};
//
TEST_F(ChannelBasicTest, SPSC_MT) {
  std::size_t N = TEST_COUNT;
  while (N--) {
    coro::SPSCChannel<int, 200000> c{};
    mt_spsc(c);
  }
}

TEST_F(ChannelBasicTest, SPSC) {
  std::size_t N = TEST_COUNT;
  while (N--) {
    {
      coro::SPSCChannel<int, 200000> c{};
      spsc(c);
    }
  }
}

// C1: Channel move must be deleted to prevent double-free
TEST(ChannelMove, IsDeleted) {
  static_assert(!std::is_move_constructible_v<coro::SPSCChannel<int, 16>>);
  static_assert(!std::is_move_assignable_v<coro::SPSCChannel<int, 16>>);
}

int main(int argc, char** argv) {
  CLI::App app{"Channel Test"};
  app.add_option("-c,--count", TEST_COUNT, "Test count");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
