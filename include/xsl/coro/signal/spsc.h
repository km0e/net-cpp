/**
 * @file spsc.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief SPSC signal for coroutines
 * @version 0.2
 * @date 2024-09-14
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_SIGNAL_SPSC
#  define XSL_CORO_SIGNAL_SPSC
#  include <xsl/coro/def.h>
#  include <xsl/coro/signal/def.h>

#  include <atomic>
#  include <coroutine>
#  include <cstddef>
#  include <functional>
#  include <limits>
#  include <utility>

XSL_CORO_NB

template <std::ptrdiff_t MaxSignals>
struct SPSCSignalStorageTag {};

struct SPSCSignalStorage {
  using max_signals
      = std::integral_constant<std::ptrdiff_t, std::numeric_limits<std::ptrdiff_t>::max()>;
  SPSCSignalStorage() : count(0), wait(false), local_count(0), callback([]() {}) {}
  std::atomic_ptrdiff_t count;
  std::atomic_flag wait;
  std::ptrdiff_t local_count;
  std::function<void()> callback;
};

template <>
struct SignalAwaiterTraits<SPSCSignalStorage> {
  using storage_type = SPSCSignalStorage;

  /// @brief Check if the signal is ready
  constexpr bool await_ready(this auto&& self) noexcept {
    self.storage.local_count = self.storage.count.fetch_sub(1, std::memory_order_acq_rel);
    return self.storage.local_count > 0;
  }
  /// @brief Suspend the signal
  template <class Promise>
  constexpr decltype(auto) await_suspend(this auto&& self, std::coroutine_handle<Promise> handle) {
    self.storage.callback = [handle] { handle.promise().resume(handle); };
    self.storage.wait.test_and_set(std::memory_order_release);
    self.storage.wait.notify_one();
  }
  /// @brief Resume the signal
  [[nodiscard("must use the result of await_resume to confirm the signal is still alive")]]
  constexpr std::size_t await_resume(this auto&& self) {
    return self.storage.local_count;  /// return the count of signals released
  }
};

template <std::ptrdiff_t MaxSignals>
struct SignalTraits<SPSCSignalStorage, MaxSignals> {
public:
  using storage_type = SPSCSignalStorage;
  using max_signals = std::integral_constant<std::ptrdiff_t, MaxSignals>;
  /**
   * @brief Release the signal
   */
  constexpr bool release(this auto& self) {
    auto local_count = self.storage.count.load(std::memory_order_relaxed);
    if (local_count == max_signals::value) {
      return false;
    }
    bool need_wake = false;
    if (local_count < 0) {
      self.storage.count.store(0, std::memory_order_relaxed);
      need_wake = true;
    } else if (self.storage.count.fetch_add(1, std::memory_order_acq_rel) < 0) {
      need_wake = true;
    }
    if (need_wake) {
      self.storage.local_count = 1;
      self.wait_and_callback();
      return true;
    }
    return false;
  }

private:
  constexpr void wait_and_callback(this auto& self) {
    self.storage.wait.wait(false, std::memory_order_acquire);
    self.storage.wait.clear(std::memory_order_relaxed);
    std::exchange(self.storage.callback, []() {})();
  }
};

XSL_CORO_NE
#endif
