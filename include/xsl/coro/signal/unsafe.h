/**
 * @file unsafe.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Unsafe signal for coroutines
 * @version 0.2
 * @date 2024-09-14
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_SIGNAL_UNSAFE
#  define XSL_CORO_SIGNAL_UNSAFE
#  include <xsl/coro/def.h>
#  include <xsl/coro/signal/def.h>
#  include <xsl/log.h>

#  include <functional>
#  include <variant>
XSL_CORO_NB

struct UnsafeSignalStorage {
  using max_signals
      = std::integral_constant<std::ptrdiff_t, std::numeric_limits<std::ptrdiff_t>::max()>;
  UnsafeSignalStorage() : state() {}
  std::variant<std::ptrdiff_t, std::function<void()>> state;
  bool stop = false;
};
template <>
struct SignalAwaiterTraits<UnsafeSignalStorage> {
  using storage_type = UnsafeSignalStorage;
  /**
   * @brief Check if the signal is ready
   *
   * @return true if the signal is ready
   * @return false if the signal is not ready
   */
  constexpr bool await_ready(this auto&& self) {
    return self.storage.stop
           || (std::holds_alternative<std::ptrdiff_t>(self.storage.state)
               && std::get<std::ptrdiff_t>(self.storage.state) > 0);
  }
  /**
   * @brief Suspend the signal
   *
   * @tparam Promise the promise type
   * @param handle the coroutine handle
   */
  template <class Promise>
  constexpr void await_suspend(this auto&& self, std::coroutine_handle<Promise> handle) {
    self.storage.state = [handle] { handle.promise().resume(handle); };
    log_trace("Signal suspended");
  }
  /**
   * @brief Resume the signal
   *
   * @return true if the signal is still alive
   * @return false if the signal is not alive
   */
  [[nodiscard("must use the result of await_resume to confirm the signal is still alive")]]
  constexpr std::size_t await_resume(this auto&& self) {
    auto& state = std::get<std::ptrdiff_t>(self.storage.state);
    if (state > 0) {
      state--;
      log_trace("Signal resumed {}", state);
      return state + 1;  // signal is still alive, return the count
    }
    return 0;  // signal is not alive
  }
};

template <std::ptrdiff_t MaxSignals>
struct SignalTraits<UnsafeSignalStorage, MaxSignals> {
  using max_signals = std::integral_constant<std::ptrdiff_t, MaxSignals>;

  using storage_type = UnsafeSignalStorage;
  /**
   * @brief Release the signal
   *
   */
  constexpr bool release(this auto&& self) {
    if (std::ptrdiff_t* state = std::get_if<std::ptrdiff_t>(&self.storage.state);
        state != nullptr) {
      *state = [](auto& cnt [[maybe_unused]]) {
        if constexpr (MaxSignals > 1) {
          return std::min(cnt + 1, MaxSignals);
        } else {
          return 1;
        }
      }(*state);
      log_trace("Signal released {}", *state);
      return false;
    } else {
      std::get<std::function<void()>>(std::exchange(self.storage.state, std::ptrdiff_t{1}))();
      log_trace("Signal Callback");
      return true;
    }
  }
  /**
   * @brief Stop the signal
   *
   * @return std::size_t the count of signals released
   */
  constexpr bool stop(this auto&& self) {
    self.storage.stop = true;
    if (std::ptrdiff_t* state = std::get_if<std::ptrdiff_t>(&self.storage.state);
        state != nullptr) {
      return false;
    }
    std::get<std::function<void()>>(std::exchange(self.storage.state, std::ptrdiff_t{0}))();
    return true;
  }
  constexpr std::optional<std::ptrdiff_t> force_stop(this auto&& self) {
    self.storage.stop = true;
    if (std::ptrdiff_t* state = std::get_if<std::ptrdiff_t>(&self.storage.state);
        state != nullptr) {
      return {std::exchange(*state, 0)};
    }
    std::get<std::function<void()>>(std::exchange(self.storage.state, std::ptrdiff_t{0}))();
    return std::nullopt;
  }
};
XSL_CORO_NE
#endif
