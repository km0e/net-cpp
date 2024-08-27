/**
 * @file tool.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_TEST_CORO_TOOL_
#  define XSL_TEST_CORO_TOOL_
#  include "xsl/coro.h"

#  include <semaphore>
#  include <stdexcept>
using namespace xsl;

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

inline Task<void> multi_task(int &value) {
  value += co_await return_task();
  co_await no_return_task(value);
}

inline Task<void> exception_penetrate_task() { co_await no_return_exception_task(); }

inline Task<void> sync_no_return_task(int &value, std::binary_semaphore &sem) {
  ++value;
  sem.release();
  co_return;
}

inline Task<void> sync_multi_task(int &value, std::binary_semaphore &sem) {
  value += co_await return_task();
  co_await sync_no_return_task(value, sem);
}

#endif
