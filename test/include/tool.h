#pragma once
#ifndef _XSL_TEST_CORO_TOOL_
#  define _XSL_TEST_CORO_TOOL_
#  include "xsl/coro/task.h"
using namespace xsl::coro;
template <class Executor = NoopExecutor>
inline Task<void, Executor> no_return_task(int &value) {
  value = 1;
  co_return;
}

template <class Executor = NoopExecutor>
inline Task<int, Executor> return_task() {
  co_return 2;
}

template <class Executor = NoopExecutor>
inline Task<void, Executor> no_return_exception_task() {
  throw std::runtime_error("error");
  co_return;
}

template <class Executor = NoopExecutor>
inline Task<int, Executor> return_exception_task() {
  throw std::runtime_error("error");
  co_return 3;
}
#endif
