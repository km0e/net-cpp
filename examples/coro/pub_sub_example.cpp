/**
 * @file pub_sub_example.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Example for Publish-Subscribe pattern for coroutines
 * @version 0.1
 * @date 2024-08-30
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "xsl/coro.h"
#include "xsl/wheel.h"

#include <memory>

/**
 * @brief Consumer
 *
 * @param r the signal receiver
 * @param sum the sum of the received values
 * @return xsl::Task<void>
 */
xsl::Task<void> consumer(xsl::SignalReceiver<> r, std::atomic<int> &sum) {
  while (co_await r) {
    sum += 1;
  }
}

int main() {
  auto executor = std::make_shared<xsl::coro::NewThreadExecutor>();
  xsl::PubSub<int, 100> pubsub{};
  std::atomic<int> sum = 0;
  for (int i = 0; i < 10; i++) {
    auto sub_res = pubsub.subscribe(i);
    xsl::rt_assert(sub_res, "sub_res");
    consumer(std::move(*sub_res), sum).detach(executor);
  }
  for (int i = 0; i < 100; i++) {
    /// publish to specific subscriber
    pubsub.publish(i);
  }
  /// publish to subscribers that satisfy the predicate
  pubsub.publish([](int v) { return v % 3 == 0; });
  pubsub.stop();
  xsl::rt_assert(sum == 14, "sum == 14");
}
