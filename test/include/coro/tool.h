#pragma once
#ifndef XSL_TEST_CORO_TOOL_
#  define XSL_TEST_CORO_TOOL_
#  include "xsl/coro/await.h"
#  include "xsl/coro/task.h"
#  include "xsl/logctl.h"

#  include <semaphore>
#  include <stdexcept>
#  include <string>
#  include <vector>
using namespace xsl::coro;
inline Task<void> resource_task(int &value) {
  std::vector<std::string> resource = {"resource"};
  LOG5("resource_task");
  co_await CallbackAwaiter<int>([](auto cb) {
    std::thread([cb]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      cb(42);
    }).detach();
  });
  LOG5("resource_task done");
  value = 1;
  co_return;
}

inline Task<void> multi_resource_task(int &value) {
  std::string resource = "resource";
  co_await resource_task(value);
  co_return;
}

inline Task<void> no_return_task(int &value) {
  ++value;
  co_return;
}

inline Task<int> return_task() { co_return 1; }

inline Task<void> no_return_exception_task() {
  throw std::runtime_error("error");
  co_return;
}

inline Task<int> return_exception_task() {
  throw std::runtime_error("error");
  co_return 1;
}

inline Task<void> sync_no_return_task(int &value, std::binary_semaphore &sem) {
  ++value;
  sem.release();
  co_return;
}

inline Task<int> sync_return_task(std::binary_semaphore &sem) {
  struct Guard {
    std::binary_semaphore &sem;
    ~Guard() { sem.release(); }
  } guard{sem};
  co_return 1;
}

#endif
