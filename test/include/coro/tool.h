#pragma once
#include <semaphore>
#include <stdexcept>
#ifndef XSL_TEST_CORO_TOOL_
#  define XSL_TEST_CORO_TOOL_
#  include "xsl/coro/await.h"
#  include "xsl/coro/task.h"
#  include "xsl/logctl.h"

#  include <string>
#  include <vector>
using namespace xsl::coro;
template <class Executor = NoopExecutor>
inline Task<void, Executor> resource_task(int &value) {
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

template <class Executor = NoopExecutor>
inline Task<void, Executor> multi_resource_task(int &value) {
  std::string resource = "resource";
  co_await resource_task(value);
  co_return;
}

template <class Executor = NoopExecutor>
inline Task<void, Executor> no_return_task(int &value) {
  ++value;
  co_return;
}

template <class Executor = NoopExecutor>
inline Task<int, Executor> return_task() {
  co_return 1;
}

template <class Executor = NoopExecutor>
inline Task<void, Executor> no_return_exception_task() {
  throw std::runtime_error("error");
  co_return;
}

template <class Executor = NoopExecutor>
inline Task<int, Executor> return_exception_task() {
  throw std::runtime_error("error");
  co_return 1;
}

template <class Executor = NoopExecutor>
inline Task<void, Executor> sync_no_return_task(int &value, std::binary_semaphore &sem) {
  ++value;
  sem.release();
  co_return;
}

template <class Executor = NoopExecutor>
inline Task<int, Executor> sync_return_task(std::binary_semaphore &sem) {
  struct Guard {
    std::binary_semaphore &sem;
    ~Guard() { sem.release(); }
  } guard{sem};
  co_return 1;
}

#endif
