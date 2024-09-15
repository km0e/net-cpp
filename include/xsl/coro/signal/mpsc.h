/**
 * @file mpsc.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief MPSC signal for coroutines
 * @version 0.1
 * @date 2024-09-14
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_SIGNAL_MPSC
#  define XSL_CORO_SIGNAL_MPSC
#  include "xsl/coro/def.h"
#  include "xsl/coro/signal/common.h"
#  include "xsl/coro/signal/unsafe.h"
#  include "xsl/wheel.h"
XSL_CORO_NB

using mpsc_max_signals = unsafe_max_signals;

struct SignalStorage {
  using unsafe_storage_type = UnsafeSignalStorage;
  SignalStorage() : mtx(), _unsafe() {}
  std::mutex mtx;
  UnsafeSignalStorage _unsafe;
};

template <>
struct SignalRxTraits<SignalStorage> {
  using storage_type = SignalStorage;
  using unsafe_traits = SignalRxTraits<UnsafeSignalStorage>;

  /**
   * @brief Check if the signal is ready
   *
   * @param storage the signal storage
   * @return true if the signal is ready
   * @return false if the signal is not ready
   */
  static constexpr bool ready(storage_type &storage) {
    storage.mtx.lock();
    return unsafe_traits::ready(storage._unsafe);
  }
  /**
   * @brief Suspend the signal
   *
   * @tparam Promise the promise type
   * @param storage the signal storage
   * @param handle the coroutine handle
   */
  template <class Promise>
  static constexpr void suspend(storage_type &storage, std::coroutine_handle<Promise> handle) {
    unsafe_traits::suspend(storage._unsafe, handle);
    storage.mtx.unlock();
    LOG6("Signal suspended");
  }
  /**
   * @brief Resume the signal
   *
   * @param storage the signal storage
   * @return true if the signal is still alive
   * @return false if the signal is not alive
   */
  [[nodiscard("must use the result of await_resume to confirm the signal is still alive")]]
  static constexpr bool resume(storage_type &storage) {
    Defer defer{[&storage] { storage.mtx.unlock(); }};
    return unsafe_traits::resume(storage._unsafe);
  }
};

template <std::ptrdiff_t MaxSignals>
struct SignalTxTraits<SignalStorage, MaxSignals> {
  using max_signals = std::integral_constant<std::ptrdiff_t, MaxSignals>;
  using unsafe_storage_type = SignalStorage::unsafe_storage_type;
  using unsafe_traits = SignalTxTraits<unsafe_storage_type, MaxSignals>;

  using storage_type = SignalStorage;
  using awaiter_type = SignalAwaiter<SignalRxTraits<storage_type>, storage_type *>;
  /**
   * @brief Release the signal
   *
   * @param storage the signal storage
   */
  static constexpr bool release(storage_type &storage) {
    storage.mtx.lock();
    bool result = unsafe_traits::release(storage._unsafe);
    if (!result) {
      storage.mtx.unlock();
    }
    return result;
  }
  /**
   * @brief Stop the signal
   *
   * @tparam Force if true, reset the signal count to 0
   * @param storage the signal storage
   * @return std::size_t the count of signals released
   */
  static constexpr bool stop(storage_type &storage) {
    storage.mtx.lock();
    bool result = unsafe_traits::stop(storage._unsafe);
    if (!result) {
      storage.mtx.unlock();
    }
    return result;
  }
  static constexpr std::optional<std::ptrdiff_t> force_stop(storage_type &storage) {
    storage.mtx.lock();
    std::optional<std::ptrdiff_t> result = unsafe_traits::force_stop(storage._unsafe);
    if (result.has_value()) {
      storage.mtx.unlock();
      return result;
    }
    return std::nullopt;
  }
};
XSL_CORO_NE
#endif
