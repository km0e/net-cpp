/**
 * @file base.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Test for signal basic functions
 * @version 0.2.2
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <CLI/CLI.hpp>
#include <gtest/gtest.h>
#include <xsl/coro.h>
#include <xsl/log.h>

#include <cassert>
#include <cstddef>
#include <semaphore>
#include <thread>
using namespace xsl;

std::size_t TEST_COUNT = 1;

class SignalBasicTest : public ::testing::Test {
protected:
  SignalBasicTest() {}

  void SetUp() override {}

  void TearDown() override {}

  ~SignalBasicTest() {}

  template <class Signal>
  void stop(Signal& sig) {
    const int N = 100;
    int count = 0;
    std::binary_semaphore prod{0}, cons{0};

    std::jthread consumer([&] {
      for (int i = 0; i < N; i++) {
        prod.acquire();  // wait for producer
        bool ok = [&] -> Task<bool> { co_return static_cast<bool>(co_await sig); }().block();
        if (ok) count++;
        cons.release();  // signal: consumed
      }
    });
    std::jthread producer([&] {
      for (int i = 0; i < N; i++) {
        sig.release();
        prod.release();  // signal: released
        cons.acquire();  // wait for consumer
      }
    });
    producer.join();
    consumer.join();
    ASSERT_EQ(count, N);
  }
};

TEST_F(SignalBasicTest, SPSC4) {
  std::size_t N = TEST_COUNT;
  while (N--) {
    {
      MPSCSignal m_sig{};
      stop(m_sig);
    }
  }
}

int main(int argc, char** argv) {
  CLI::App app{"Echo server"};
  app.add_option("-c,--count", TEST_COUNT, "Test count");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
