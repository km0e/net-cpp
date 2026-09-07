/**
 * @file test_pub_sub.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Test for Publish-Subscribe pattern for coroutines
 * @version 0.4.0
 * @date 2024-08-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "random.h"

#include <gtest/gtest.h>
#include <xsl/coro.h>

#include <chrono>
#include <semaphore>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace xsl;

TEST(ExactPubSub, Exit) {
  auto pubsub = coro::make_pub_sub<_value_pack<1>, MPSCSignal>();
  auto* sig = pubsub->template signal<1>();
  ASSERT_NE(sig, nullptr);

  int count = 0;
  std::binary_semaphore ready{0}, done{0};

  [](auto sub, auto* sig, int& count, std::binary_semaphore& ready,
     std::binary_semaphore& done) -> Task<void> {
    while (true) {
      ready.release();
      if (!co_await *sig) break;
      count++;
    }
    done.release();
  }(std::move(pubsub), sig, count, ready, done)
                                         .detach(coro::CoroContext(coro::NewThreadExecutor{}));

  ready.acquire();  // consumer suspended in await
  sig->release();
  ready.acquire();  // consumer consumed, looped, re-suspended
  sig->stop();
  done.acquire();  // consumer exited while loop
  ASSERT_GE(count, 1);
}


int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
