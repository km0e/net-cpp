/**
 * @file test_signal.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Test for signal
 * @version 0.22
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "CLI/CLI.hpp"
#include "xsl/coro.h"
#include "xsl/logctl.h"

#include <gtest/gtest.h>

#include <cassert>
#include <cstddef>
#include <thread>
using namespace xsl;

std::size_t TEST_COUNT = 1;

class SignalCommonTest : public ::testing::Test {
protected:
  SignalCommonTest() {}

  void SetUp() override {}

  void TearDown() override {}

  ~SignalCommonTest() {}

  template <class Signal>
  void pin(Signal &sig) {
    auto pin = sig.pin();
    auto moved = std::move(sig);
    ASSERT_TRUE(pin);
    pin.release();
    ASSERT_TRUE([&pin] -> Task<bool> { co_return co_await pin; }().block());
  }

  template <class Signal>
  void stop(Signal &sig) {
    const int N = 100000;
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
    // INFO("call stop");
    sig.stop();
    consumer.join();
    xsl::flush_log();
    ASSERT_EQ(count, N);
  }

  template <class Signal>
  void force_stop(Signal &sig) {
    const int N = 100000;
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
    // INFO("call force_stop");
    auto rest = sig.force_stop();
    std::ptrdiff_t cnt = rest.has_value() ? *rest : 0;
    consumer.join();
    xsl::flush_log();
    ASSERT_EQ(cnt + count, N);
  }
};

TEST_F(SignalCommonTest, MPSC) {
  std::size_t N = TEST_COUNT;
  while (N--) {
    {
      Signal<1> sig{};
      pin(sig);
    }
    {
      Signal<> m_sig{};
      stop(m_sig);
    }
    {
      Signal<> m_sig{};
      force_stop(m_sig);
    }
  }
}

TEST_F(SignalCommonTest, SPSC) {
  std::size_t N = TEST_COUNT;
  while (N--) {
    {
      SPSCSignal<1> sig{};
      pin(sig);
    }
    {
      SPSCSignal<> m_sig{};
      stop(m_sig);
    }
    {
      SPSCSignal<> m_sig{};
      force_stop(m_sig);
    }
  }
}

int main(int argc, char **argv) {
  CLI::App app{"Echo server"};
  app.add_option("-c,--count", TEST_COUNT, "Test count");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
