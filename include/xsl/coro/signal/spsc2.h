/**
 * @file spsc2.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Single Producer Single Consumer Signal
 * @version 0.1
 * @date 2025-06-01
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#ifndef XSL_CORO_ASYNC_VAR
#  define XSL_CORO_ASYNC_VAR

#  include "xsl/coro/def.h"

#  include <atomic>
#  include <cassert>
#  include <cstddef>
#  include <functional>
#  include <limits>
#  include <optional>
XSL_CORO_NB
const std::size_t BASE_SHIFT = 1;
static_assert(BASE_SHIFT > 0, "BASE_SHIFT must be greater than 0");

const std::ptrdiff_t STOP_MASK = 1 << (BASE_SHIFT - 1);

using spsc_max_signals
    = std::integral_constant<std::ptrdiff_t,
                             (std::numeric_limits<std::ptrdiff_t>::max() >> BASE_SHIFT) - 1>;

struct SPSCSignalStorage2 {
  SPSCSignalStorage2() : count(0), callback(nullptr), local_count(0) {}
  std::atomic_ptrdiff_t count;
  std::atomic<std::function<void()> *> callback;
  std::ptrdiff_t local_count;
};

struct SignalAwaiter2 {
  using storage_type = SPSCSignalStorage2;

  storage_type &storage;

  /// @brief Check if the signal is ready
  constexpr bool await_ready() noexcept {
    storage.local_count = storage.count.fetch_sub(1 << BASE_SHIFT, std::memory_order_acq_rel);
    return storage.local_count > 0;
  }
  /// @brief Suspend the signal
  template <class Promise>
  constexpr decltype(auto) await_suspend(std::coroutine_handle<Promise> handle) {
    storage.callback.store(
        new std::function<void()>([handle]() { handle.promise().resume(handle); }),
        std::memory_order_release);
    storage.callback.notify_one();
  }
  /// @brief Resume the signal
  [[nodiscard("must use the result of await_resume to confirm the signal is still alive")]]
  constexpr bool await_resume() {
    return storage.local_count > STOP_MASK;
  }
};

template <std::ptrdiff_t MaxSignals>
struct Signal2 {
  using max_signals = std::integral_constant<std::ptrdiff_t, MaxSignals>;

  using storage_type = SPSCSignalStorage2;
  using awaiter_type = SignalAwaiter2;

  storage_type storage;

  Signal2() : storage() {}

  awaiter_type operator co_await() { return awaiter_type{this->storage}; }
  /**
   * @brief Release the signal
   *
   * @param storage the signal storage
   */
  constexpr bool release() {
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
  constexpr bool stop() {
    auto cnt = storage.count.fetch_or(STOP_MASK, std::memory_order_acq_rel);
    if ((!(cnt & STOP_MASK)) && cnt < 0) {  /// if stop flag has been set, return false
      wait_and_callback(storage);
      return true;
    }
    return false;
  }
  constexpr std::optional<std::ptrdiff_t> force_stop() {
    auto cnt = storage.count.exchange(STOP_MASK, std::memory_order_acq_rel);
    if (cnt < 0) {
      wait_and_callback(storage);
      return std::nullopt;
    }
    return cnt >> BASE_SHIFT;
  }

private:
  static constexpr void wait_and_callback(storage_type &storage) {
    storage.callback.wait(nullptr, std::memory_order_acquire);
    auto f = storage.callback.exchange(nullptr, std::memory_order_relaxed);
    assert(f != nullptr && "Signal callback is null");
    (*f)();    // call the callback function
    delete f;  // delete the callback function
  }
};

XSL_CORO_NE
#endif
