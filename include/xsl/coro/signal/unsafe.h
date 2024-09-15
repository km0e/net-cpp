/**
 * @file unsafe.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Unsafe signal for coroutines
 * @version 0.1
 * @date 2024-09-14
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_SIGNAL_UNSAFE
#  define XSL_CORO_SIGNAL_UNSAFE
#  include "xsl/coro/def.h"
#  include "xsl/coro/signal/common.h"
#  include "xsl/logctl.h"

#  include <functional>
#  include <variant>
XSL_CORO_NB

using unsafe_max_signals
    = std::integral_constant<std::ptrdiff_t, std::numeric_limits<std::ptrdiff_t>::max()>;

struct UnsafeSignalStorage {
  UnsafeSignalStorage() : state() {}
  std::variant<std::ptrdiff_t, std::function<void()>> state;
  bool stop = false;
};

template <>
struct SignalRxTraits<UnsafeSignalStorage> {
  using storage_type = UnsafeSignalStorage;
  /**
   * @brief Check if the signal is ready
   *
   * @param storage the signal storage
   * @return true if the signal is ready
   * @return false if the signal is not ready
   */
  static constexpr bool ready(storage_type &storage) {
    return storage.stop
           || (std::holds_alternative<std::ptrdiff_t>(storage.state)
               && std::get<std::ptrdiff_t>(storage.state) > 0);
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
    storage.state = [handle] { handle.promise().resume(handle); };
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
    auto &state = std::get<std::ptrdiff_t>(storage.state);
    if (state > 0) {
      state--;
      LOG6("Signal resumed {}", state);
      return true;
    }
    return false;
  }
};

template <std::ptrdiff_t MaxSignals>
struct SignalTxTraits<UnsafeSignalStorage, MaxSignals> {
  using max_signals = std::integral_constant<std::ptrdiff_t, MaxSignals>;

  using storage_type = UnsafeSignalStorage;
  using awaiter_type = SignalAwaiter<SignalRxTraits<storage_type>, storage_type *>;
  /**
   * @brief Release the signal
   *
   * @param storage the signal storage
   */
  static constexpr bool release(storage_type &storage) {
    if (std::ptrdiff_t *state = std::get_if<std::ptrdiff_t>(&storage.state); state != nullptr) {
      *state = [](auto &cnt [[maybe_unused]]) {
        if constexpr (MaxSignals > 1) {
          return std::min(cnt + 1, MaxSignals);
        } else {
          return 1;
        }
      }(*state);
      LOG6("Signal released {}", *state);
      return false;
    } else {
      std::get<std::function<void()>>(std::exchange(storage.state, std::ptrdiff_t{1}))();
      LOG6("Signal Callback");
      return true;
    }
  }
  /**
   * @brief Stop the signal
   *
   * @tparam Force if true, reset the signal count to 0
   * @param storage the signal storage
   * @return std::size_t the count of signals released
   */
  static constexpr bool stop(storage_type &storage) {
    storage.stop = true;
    if (std::ptrdiff_t *state = std::get_if<std::ptrdiff_t>(&storage.state); state != nullptr) {
      return false;
    }
    std::get<std::function<void()>>(std::exchange(storage.state, std::ptrdiff_t{0}))();
    return true;
  }
  static constexpr std::optional<std::ptrdiff_t> force_stop(storage_type &storage) {
    storage.stop = true;
    if (std::ptrdiff_t *state = std::get_if<std::ptrdiff_t>(&storage.state); state != nullptr) {
      return {std::exchange(*state, 0)};
    }
    std::get<std::function<void()>>(std::exchange(storage.state, std::ptrdiff_t{0}))();
    return std::nullopt;
  }
};
XSL_CORO_NE
#endif
