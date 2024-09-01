# Signal Example {#signal_example}
This example shows how to use the `signal` simply.

```cpp

#include "xsl/coro.h"
#include "xsl/wheel.h"
const int N = 100000;
/**
 * @brief Simple producer
 *
 * @param w
 * @return xsl::Task<void>
 */
xsl::Task<void> simple_producer(xsl::SignalSender<N> w) {
  for (int i = 0; i < 100000; i++) {
    w.release();
  }
  w.stop();
  co_return;
}
/**
 * @brief Force producer
 *
 * @param w
 * @return xsl::Task<void>
 */
xsl::Task<void> force_producer(xsl::SignalSender<N> w) {
  for (int i = 0; i < 100000; i++) {
    w.release();
  }
  w.stop<true>();
  co_return;
}
/**
 * @brief Consumer
 *
 * @param r
 * @return xsl::Task<int>
 */
xsl::Task<int> consumer(xsl::SignalReceiver<> r) {
  int count = 0;
  while (co_await r) {
    ++count;
  }
  co_return count;
}

int main() {
  auto [r, w] = xsl::signal<N>();
  auto exec = std::make_shared<xsl::coro::NewThreadExecutor>();

  simple_producer(std::move(w)).detach(exec);
  int count = consumer(std::move(r)).block();
  xsl::dynamic_assert(count == N, "count == N");

  auto [r2, w2] = xsl::signal<N>();
  force_producer(std::move(w2)).detach(exec);
  count = consumer(std::move(r2)).block();
  xsl::dynamic_assert(count <= N, "count <= N");
}
```
