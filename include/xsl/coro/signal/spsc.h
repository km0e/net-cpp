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
#  include <optional>
#  include <utility>

XSL_CORO_NB

const std::size_t BASE_SHIFT = 1;
static_assert(BASE_SHIFT > 0, "BASE_SHIFT must be greater than 0");

const std::ptrdiff_t STOP_MASK = 1 << (BASE_SHIFT - 1);

struct SPSCSignalStorage {
  using max_signals
      = std::integral_constant<std::ptrdiff_t,
                               (std::numeric_limits<std::ptrdiff_t>::max() >> BASE_SHIFT) - 1>;
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
  constexpr bool await_ready(this auto &&self) noexcept {
    self.storage.local_count
        = self.storage.count.fetch_sub(1 << BASE_SHIFT, std::memory_order_acq_rel);
    return self.storage.local_count > 0;
  }
  /// @brief Suspend the signal
  template <class Promise>
  constexpr decltype(auto) await_suspend(this auto &&self, std::coroutine_handle<Promise> handle) {
    self.storage.callback = [handle] { handle.promise().resume(handle); };
    self.storage.wait.test_and_set(std::memory_order_release);
    self.storage.wait.notify_one();
  }
  /// @brief Resume the signal
  [[nodiscard("must use the result of await_resume to confirm the signal is still alive")]]
  constexpr std::size_t await_resume(this auto &&self) {
    if (self.storage.local_count > STOP_MASK) {
      return self.storage.local_count >> BASE_SHIFT;  /// return the count of signals released
    }
    return 0;  /// signal is not alive
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
  constexpr bool release(this auto &self) {
    auto local_count = self.storage.count.load(std::memory_order_relaxed);
    if ((local_count >> BASE_SHIFT) == max_signals::value) {
      return false;
    }
    if (local_count < 0) {
      self.storage.count.store(0, std::memory_order_relaxed);
      goto wait_callback;
    }
    if (self.storage.count.fetch_add(1 << BASE_SHIFT, std::memory_order_acq_rel) < 0) {
      goto wait_callback;
    }
    return false;

  wait_callback:  /// obviously, goto is better than if-else here
    self.storage.local_count
        = 1 << BASE_SHIFT;  /// set local count to 2 to indicate the signal not stopped
    self.wait_and_callback();

    return true;
  }
  /**
   * @brief Stop the signal
   *
   * @tparam Force if true, reset the signal count to 0
   * @param storage the signal storage
   * @return std::size_t the count of signals released
   */
  constexpr bool stop(this auto &self) {
    auto cnt = self.storage.count.fetch_or(STOP_MASK, std::memory_order_acq_rel);
    if ((!(cnt & STOP_MASK)) && cnt < 0) {  /// if stop flag has been set, return false
      self.wait_and_callback();
      return true;
    }
    return false;
  }
  constexpr std::optional<std::ptrdiff_t> force_stop(this auto &self) {
    auto cnt = self.storage.count.exchange(STOP_MASK, std::memory_order_acq_rel);
    if (cnt < 0) {
      self.wait_and_callback();
      return std::nullopt;
    }
    return cnt >> BASE_SHIFT;
  }

private:
  constexpr void wait_and_callback(this auto &self) {
    self.storage.wait.wait(false, std::memory_order_acquire);
    self.storage.wait.clear(std::memory_order_relaxed);
    std::exchange(self.storage.callback, []() {})();
  }
};

XSL_CORO_NE
#endif
