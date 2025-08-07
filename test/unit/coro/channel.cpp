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
using namespace xsl;

std::size_t TEST_COUNT = 1;

class ChannelCommonTest : public ::testing::Test {
protected:
  ChannelCommonTest() {}

  void SetUp() override {}

  void TearDown() override {}

  ~ChannelCommonTest() {}

  template <class Channel>
  void spsc(Channel &sig) {
    const int N = 100000;
    int count = 0;
    std::jthread consumer([&] {
      for (int i = 0; i < N; i++) {
        count += [&] -> Task<int> { co_return co_await sig; }().block();
      }
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

TEST_F(ChannelCommonTest, SPSC) {
  std::size_t N = TEST_COUNT;
  while (N--) {
    {
      coro::SPSCChannel<int, 200000> c{};
      spsc(c);
    }
  }
}

int main(int argc, char **argv) {
  CLI::App app{"Channel Test"};
  app.add_option("-c,--count", TEST_COUNT, "Test count");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
