/**
 * @file signal.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Signal for coroutines
 * @version 0.5.0
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#ifndef XSL_CORO_SIGNAL
#  define XSL_CORO_SIGNAL
#  include <xsl/coro/def.h>
#  include <xsl/coro/signal/def.h>
#  include <xsl/coro/signal/mpsc.h>
#  include <xsl/coro/signal/spsc.h>
#  include <xsl/coro/signal/spsc2.h>
#  include <xsl/coro/signal/spsc4.h>
#  include <xsl/coro/signal/unsafe.h>

#  include <cassert>
#  include <cstddef>

XSL_CORO_NB
template <std::ptrdiff_t MaxSignals = UnsafeSignalStorage::max_signals::value>
using UnsafeSignal = AnySignal<UnsafeSignalStorage, MaxSignals>;

template <std::ptrdiff_t MaxSignals = UnsafeSignalStorage::max_signals::value>
using Signal = AnySignal<SignalStorage, MaxSignals>;

template <std::ptrdiff_t MaxSignals = SPSCSignalStorage::max_signals::value>
using SPSCSignal = AnySignal<SPSCSignalStorage, MaxSignals>;

XSL_CORO_NE
#endif
