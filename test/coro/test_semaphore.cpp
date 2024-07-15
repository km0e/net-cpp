#include "xsl/coro/semaphore.h"
#include "xsl/coro/task.h"

#include <gtest/gtest.h>

#include <cassert>
#include <thread>
using namespace xsl::coro;

TEST(SemaphoreTest, Basic) {
  CountingSemaphore<1> sem{};
  int value = 0;
  std::thread t([&] {
    [&]() -> Task<void> {
      co_await sem;
      assert(value == 1);
      co_return;
    }()
                 .block();
  });
  value = 1;
  sem.release();
  t.join();
};

int main(int argc, char **argv) {
  xsl::no_log();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
