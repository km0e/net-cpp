/**
 * @file coro.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Coroutine utilities
 * @version 0.2
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO
#  define XSL_CORO
#  include "xsl/coro/block.h"
#  include "xsl/coro/detach.h"
#  include "xsl/coro/executor.h"
#  include "xsl/coro/guard.h"
#  include "xsl/coro/pub_sub.h"
#  include "xsl/coro/signal.h"
#  include "xsl/coro/signal/common.h"
#  include "xsl/coro/task.h"
#  include "xsl/def.h"
XSL_NB
namespace coro {
  using _coro::ExecutorBase;
  using _coro::NewThreadExecutor;
  using _coro::NoopExecutor;
}  // namespace coro
// using _coro::BinarySignal;
using _coro::AnySignal;
using _coro::block;
using _coro::detach;

using _coro::Signal;
using _coro::SignalAwaiter;
using _coro::UnsafeSignal;
// using _coro::UnsafeBinarySignal;
using _coro::SPSCSignal;
// using _coro::UnsafeBinarySignal;

using _coro::ExactPubSub;
using _coro::make_exact_pub_sub;
using _coro::PubSub;
// using _coro::Subscriber;

using _coro::ArgGuard;
using _coro::Task;
XSL_NE
#endif
