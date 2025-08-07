/**
 * @file spsc2.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Single Producer Single Consumer Signal
 * @version 0.2
 * @date 2025-06-01
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#ifndef XSL_CORO_SIGNAL_SPSC2
#  define XSL_CORO_SIGNAL_SPSC2

#  include <xsl/coro/def.h>
#  include <xsl/coro/signal/def.h>

#  include <atomic>
#  include <cassert>
#  include <cstddef>
#  include <functional>
#  include <limits>
#  include <optional>
XSL_CORO_NB
const std::size_t BASE_SHIFT2 = 1;
static_assert(BASE_SHIFT2 > 0, "BASE_SHIFT must be greater than 0");

const std::ptrdiff_t STOP_MASK2 = 1 << (BASE_SHIFT2 - 1);

struct SPSCSignalStorage2 {
  using max_signals
      = std::integral_constant<std::ptrdiff_t,
                               (std::numeric_limits<std::ptrdiff_t>::max() >> BASE_SHIFT2) - 1>;
  SPSCSignalStorage2() : count(0), callback(nullptr), local_count(0) {}
  std::atomic_ptrdiff_t count;
  std::atomic<std::function<void()> *> callback;
  std::ptrdiff_t local_count;
};

template <>
struct SignalAwaiterTraits<SPSCSignalStorage2> {
  using storage_type = SPSCSignalStorage2;

  /// @brief Check if the signal is ready
  constexpr bool await_ready(this auto &self) noexcept {
    self.storage.local_count
        = self.storage.count.fetch_sub(1 << BASE_SHIFT2, std::memory_order_acq_rel);
    return self.storage.local_count > 0;
  }
  /// @brief Suspend the signal
  template <class Promise>
  constexpr decltype(auto) await_suspend(this auto &self, std::coroutine_handle<Promise> handle) {
    self.storage.callback.store(
        new std::function<void()>([handle]() { handle.promise().resume(handle); }),
        std::memory_order_release);
    self.storage.callback.notify_one();
  }
  /// @brief Resume the signal
  [[nodiscard("must use the result of await_resume to confirm the signal is still alive")]]
  constexpr std::size_t await_resume(this auto &self) {
    return self.storage.local_count >> BASE_SHIFT2;  /// return the count of signals released
  }
};

template <std::ptrdiff_t MaxSignals>
struct SignalTraits<SPSCSignalStorage2, MaxSignals> {
public:
  using storage_type = SPSCSignalStorage2;
  using max_signals = std::integral_constant<std::ptrdiff_t, MaxSignals>;

  /**
   * @brief Release the signal
   *
   */
  constexpr bool release(this auto &self) {
    auto local_count = self.storage.count.load(std::memory_order_relaxed);
    if ((local_count >> BASE_SHIFT2) == max_signals::value) {
      return false;
    }
    if (local_count < 0) {
      self.storage.count.store(0, std::memory_order_relaxed);
      goto wait_callback;
    }
    if (self.storage.count.fetch_add(1 << BASE_SHIFT2, std::memory_order_acq_rel) < 0) {
      goto wait_callback;
    }
    return false;

  wait_callback:  /// obviously, goto is better than if-else here
    self.storage.local_count
        = 1 << BASE_SHIFT2;  /// set local count to 2 to indicate the signal not stopped
    self.wait_and_callback();

    return true;
  }
  /**
   * @brief Stop the signal
   *
   * @return
   */
  constexpr bool stop(this auto &self) {
    auto cnt = self.storage.count.fetch_or(STOP_MASK2, std::memory_order_acq_rel);
    if ((!(cnt & STOP_MASK2)) && cnt < 0) {  /// if stop flag has been set, return false
      self.wait_and_callback();
      return true;
    }
    return false;
  }
  constexpr std::optional<std::ptrdiff_t> force_stop(this auto &self) {
    auto cnt = self.storage.count.exchange(STOP_MASK2, std::memory_order_acq_rel);
    if (cnt < 0) {
      self.wait_and_callback();
      return std::nullopt;
    }
    return cnt >> BASE_SHIFT2;
  }

private:
  constexpr void wait_and_callback(this auto &self) {
    self.storage.callback.wait(nullptr, std::memory_order_acquire);
    auto f = self.storage.callback.exchange(nullptr, std::memory_order_relaxed);
    assert(f != nullptr && "Signal callback is null");
    (*f)();    // call the callback function
    delete f;  // delete the callback function
  }
};

XSL_CORO_NE
#endif
