/**
 * @file signal.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Signal for coroutines
 * @version 0.3
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_SIGNAL
#  define XSL_CORO_SIGNAL
#  include "xsl/coro/def.h"
#  include "xsl/coro/signal/mpsc.h"
#  include "xsl/coro/signal/spsc.h"
#  include "xsl/coro/signal/unsafe.h"

#  include <cassert>
#  include <cstddef>
#  include <memory>

XSL_CORO_NB
template <std::ptrdiff_t MaxSignals = unsafe_max_signals::value,
          class TxTraits = SignalTxTraits<UnsafeSignalStorage, MaxSignals>,
          class Pointer = std::shared_ptr<typename TxTraits::storage_type>>
using UnsafeSignal = AnySignal<TxTraits, Pointer>;

template <std::ptrdiff_t MaxSignals = mpsc_max_signals::value,
          class TxTraits = SignalTxTraits<SignalStorage, MaxSignals>,
          class Pointer = std::shared_ptr<typename TxTraits::storage_type>>
using Signal = AnySignal<TxTraits, Pointer>;

template <std::ptrdiff_t MaxSignals = spsc_max_signals::value,
          class TxTraits = SignalTxTraits<SPSCSignalStorage, MaxSignals>,
          class Pointer = std::shared_ptr<typename TxTraits::storage_type>>
using SPSCSignal = AnySignal<TxTraits, Pointer>;

XSL_CORO_NE
#endif
