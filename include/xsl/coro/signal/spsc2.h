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

#  include <xsl/compose.h>
#  include <xsl/coro/def.h>
#  include <xsl/coro/signal/def.h>

#  include <algorithm>
#  include <atomic>
#  include <cassert>
#  include <cstddef>
#  include <functional>
#  include <limits>
#  include <memory>
#  include <optional>
XSL_CORO_NB
const std::size_t BASE_SHIFT2 = 1;
static_assert(BASE_SHIFT2 > 0, "BASE_SHIFT must be greater than 0");

const std::ptrdiff_t STOP_MASK2 = 1 << (BASE_SHIFT2 - 1);

template <std::ptrdiff_t MaxSignals>
struct SPSCSignalStorage2Tag {};

template <std::ptrdiff_t MaxSignals>
struct SPSCSignalStorage2 {
  using max_signals = std::integral_constant<
      std::ptrdiff_t,
      std::min(MaxSignals, (std::numeric_limits<std::ptrdiff_t>::max() >> BASE_SHIFT2) - 1)>;
  std::atomic_ptrdiff_t count = 0;  /// the lower bits are used to indicate the stop flag
  std::atomic<std::function<void()>*> callback = nullptr;
  std::ptrdiff_t local_count = 0;  /// local count of signals

  /// @brief Check if the signal is ready
  constexpr bool await_ready(this auto& self) noexcept {
    self.local_count = self.count.fetch_sub(1 << BASE_SHIFT2, std::memory_order_acq_rel);
    return self.local_count > 0;
  }
  /// @brief Suspend the signal
  template <class Promise>
  constexpr decltype(auto) await_suspend(this auto& self, std::coroutine_handle<Promise> handle) {
    self.callback.store(new std::function<void()>([handle]() { handle.promise().resume(handle); }),
                        std::memory_order_release);
    self.callback.notify_one();
  }
  /// @brief Resume the signal
  constexpr std::size_t await_resume(this auto& self) {
    return self.local_count >> coro::BASE_SHIFT2;  /// return the count of signals released
  }
  /**
   * @brief Release the signal
   *
   */
  constexpr bool release(this auto& self) {
    auto local_count = self.count.load(std::memory_order_relaxed);
    if ((local_count >> BASE_SHIFT2) == max_signals::value) {
      return false;
    }
    bool need_wake = false;
    if (local_count < 0) {
      self.count.store(0, std::memory_order_relaxed);
      need_wake = true;
    } else if (self.count.fetch_add(1 << BASE_SHIFT2, std::memory_order_acq_rel) < 0) {
      need_wake = true;
    }
    if (need_wake) {
      self.local_count = 1 << BASE_SHIFT2;
      self.wait_and_callback();
      return true;
    }
    return false;
  }
  /**
   * @brief Stop the signal
   *
   * @return
   */
  constexpr bool stop(this auto& self) {
    auto cnt = self.count.fetch_or(STOP_MASK2, std::memory_order_acq_rel);
    if ((!(cnt & STOP_MASK2)) && cnt < 0) {  /// if stop flag has been set, return false
      self.wait_and_callback();
      return true;
    }
    return false;
  }
  constexpr std::optional<std::ptrdiff_t> force_stop(this auto& self) {
    auto cnt = self.count.exchange(STOP_MASK2, std::memory_order_acq_rel);
    if (cnt < 0) {
      self.wait_and_callback();
      return std::nullopt;
    }
    return cnt >> BASE_SHIFT2;
  }

private:
  constexpr void wait_and_callback(this auto& self) {
    self.callback.wait(nullptr, std::memory_order_acquire);
    auto f = self.callback.exchange(nullptr, std::memory_order_relaxed);
    assert(f != nullptr && "Signal callback is null");
    auto _ = std::unique_ptr<std::function<void()>>(f);  // scope guard
    (*f)();
  }
};


template <std::ptrdiff_t MaxSignals
          = (std::numeric_limits<std::ptrdiff_t>::max() >> BASE_SHIFT2) - 1>
using SPSCSignal2 = SPSCSignalStorage2<MaxSignals>;

XSL_CORO_NE
XSL_NB
namespace {
  using coro::BASE_SHIFT2;
  using coro::STOP_MASK2;
}  // namespace

XSL_NE
#endif
