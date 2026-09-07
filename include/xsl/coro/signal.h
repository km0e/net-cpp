/**
 * @file signal.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Signal for coroutines — unified core, two contention tiers
 * @version 0.6.0
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 * The signal family was consolidated into a single state machine
 * (include/xsl/coro/signal/core.h):
 *
 *   - UnsafeSignal: single-threaded, zero synchronization
 *   - MPSCSignal:   lock-free atomic; release()/stop() are safe from any
 *                   number of producer threads (SPSC is the degenerate case).
 *                   Cancellation via Cancellable<> is only valid on this tier.
 *
 * Both share identical semantics: await_resume() returns true on signal and
 * false after stop(); stop() is sticky; releases coalesce.
 */
#pragma once

#ifndef XSL_CORO_SIGNAL
#  define XSL_CORO_SIGNAL
#  include <xsl/coro/def.h>
#  include <xsl/coro/signal/core.h>

#  include <cassert>
#  include <cstddef>

#endif
