/**
 * @file spsc.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief SPSC signal for coroutines
 * @version 0.1
 * @date 2024-09-14
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_SIGNAL_SPSC
#  define XSL_CORO_SIGNAL_SPSC
#  include "xsl/coro/def.h"
#  include "xsl/coro/signal/common.h"

#  include <atomic>
#  include <coroutine>
#  include <cstddef>
#  include <functional>
#  include <optional>
#  include <utility>
XSL_CORO_NB
const std::size_t BASE_SHIFT = 1;
static_assert(BASE_SHIFT > 0, "BASE_SHIFT must be greater than 0");

const std::ptrdiff_t STOP_MASK = 1 << (BASE_SHIFT - 1);

using spsc_max_signals
    = std::integral_constant<std::ptrdiff_t,
                             (std::numeric_limits<std::ptrdiff_t>::max() >> BASE_SHIFT) - 1>;

struct SPSCSignalStorage {
  SPSCSignalStorage() : count(0), wait(false), local_count(0), callback([]() {}) {}
  std::atomic_ptrdiff_t count;
  std::atomic_flag wait;
  std::ptrdiff_t local_count;
  std::function<void()> callback;
};

template <>
struct SignalRxTraits<SPSCSignalStorage> {
  using storage_type = SPSCSignalStorage;
  /**
   * @brief Check if the signal is ready
   *
   * @param storage the signal storage
   * @return true if the signal is ready
   * @return false if the signal is not ready
   */
  static bool ready(storage_type &storage) {
    storage.local_count = storage.count.fetch_sub(1 << BASE_SHIFT, std::memory_order_acq_rel);
    return storage.local_count > 0;
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
    storage.callback = [handle] { handle.promise().resume(handle); };
    storage.wait.test_and_set(std::memory_order_release);
    storage.wait.notify_one();
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
    return storage.local_count > (1 << (BASE_SHIFT - 1));
  }
};

template <std::ptrdiff_t MaxSignals>
struct SignalTxTraits<SPSCSignalStorage, MaxSignals> {
  using max_signals = std::integral_constant<std::ptrdiff_t, MaxSignals>;

  using storage_type = SPSCSignalStorage;
  using awaiter_type = SignalAwaiter<SignalRxTraits<storage_type>, storage_type *>;
  /**
   * @brief Release the signal
   *
   * @param storage the signal storage
   */
  static constexpr bool release(storage_type &storage) {
    auto local_count = storage.count.load(std::memory_order_relaxed);
    if ((local_count >> BASE_SHIFT) == max_signals::value) {
      return false;
    }
    if (local_count < 0) {
      storage.count.store(0, std::memory_order_relaxed);
      goto wait_callback;
    }
    if (storage.count.fetch_add(1 << BASE_SHIFT, std::memory_order_acq_rel) < 0) {
      goto wait_callback;
    }
    return false;

  wait_callback:  /// obviously, goto is better than if-else here
    storage.local_count
        = 1 << BASE_SHIFT;  /// set local count to 2 to indicate the signal not stopped
    wait_and_callback(storage);

    return true;
  }
  /**
   * @brief Stop the signal
   *
   * @tparam Force if true, reset the signal count to 0
   * @param storage the signal storage
   * @return std::size_t the count of signals released
   */
  static constexpr bool stop(storage_type &storage) {
    auto cnt = storage.count.fetch_or(STOP_MASK, std::memory_order_acq_rel);
    if ((!(cnt & STOP_MASK)) && cnt < 0) {  /// if stop flag has been set, return false
      wait_and_callback(storage);
      return true;
    }
    return false;
  }
  static constexpr std::optional<std::ptrdiff_t> force_stop(storage_type &storage) {
    auto cnt = storage.count.exchange(STOP_MASK, std::memory_order_acq_rel);
    if (cnt < 0) {
      wait_and_callback(storage);
      return std::nullopt;
    }
    return cnt >> BASE_SHIFT;
  }

private:
  static constexpr void wait_and_callback(storage_type &storage) {
    storage.wait.wait(false, std::memory_order_acquire);
    storage.wait.clear(std::memory_order_relaxed);
    std::exchange(storage.callback, []() {})();
  }
};
XSL_CORO_NE
#endif
