/**
 * @file coro.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Coroutine utilities
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO
#  define XSL_CORO
#  include "xsl/coro/await.h"
#  include "xsl/coro/executor.h"
#  include "xsl/coro/signal.h"
#  include "xsl/coro/task.h"
#  include "xsl/def.h"
XSL_NB
namespace coro {
  using _coro::ExecutorBase;
  using _coro::GetExecutor;
  using _coro::NewThreadExecutor;
  using _coro::NoopExecutor;
}  // namespace coro
using _coro::BinarySignal;
using _coro::Signal;
using _coro::Task;
using _coro::UnsafeBinarySignal;
using _coro::UnsafeSignal;
XSL_NE
#endif
