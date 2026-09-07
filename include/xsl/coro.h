/**
 * @file coro.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Coroutine utilities
 * @version 0.3.0
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO
#  define XSL_CORO
#  include <xsl/coro/channel.h>
#  include <xsl/coro/core/base.h>
#  include <xsl/coro/core/block.h>
#  include <xsl/coro/core/context.h>
#  include <xsl/coro/core/def.h>
#  include <xsl/coro/core/detach.h>
#  include <xsl/coro/core/executor.h>
#  include <xsl/coro/core/task.h>
#  include <xsl/coro/core/then.h>
#  include <xsl/coro/error.h>
#  include <xsl/coro/guard.h>
#  include <xsl/coro/pub_sub.h>
#  include <xsl/coro/signal.h>
#  include <xsl/def.h>
XSL_NB
// Re-export the coro public API explicitly (a using-directive here would also
// pull every future coro member into xsl silently)
using coro::block;
using coro::detach;
using coro::make_pub_sub;
using coro::noop_executor;
using coro::ArgGuard;
using coro::ArgGuardAwaiter;
using coro::Awaitable;
using coro::AwaiterWrapper;
using coro::CoroContext;
using coro::Detach;
using coro::Executor;
using coro::ExecutorBase;
using coro::ForceReleasable;
using coro::MPSCSignal;
using coro::NewThreadExecutor;
using coro::NoopExecutor;
using coro::noop_coroutine;
using coro::PubSubUtil;
using coro::Reserved;
using coro::SPSCChannel;
using coro::SPSCChannelStorage;
using coro::StaticExactPubSubStorage;
using coro::ExactPubSubStorage;
using coro::Task;
using coro::ThenAwaiter;
using coro::ThreadPoolExecutor;
using coro::UnsafeSignal;
XSL_NE
#endif
