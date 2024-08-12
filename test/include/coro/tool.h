#pragma once
#ifndef XSL_TEST_CORO_TOOL_
#  define XSL_TEST_CORO_TOOL_
#  include "xsl/coro.h"

#  include <semaphore>
#  include <stdexcept>
using namespace xsl::coro;

inline Task<void> no_return_task(int &value) {
  ++value;
  co_return;
}

inline Lazy<void> no_return_lazy(int &value) {
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

inline Lazy<void> sync_no_return_lazy(int &value, std::binary_semaphore &sem) {
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
