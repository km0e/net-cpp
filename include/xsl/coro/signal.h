/**
 * @file signal.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Signal for coroutines
 * @version 0.2
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_SIGNAL
#  define XSL_CORO_SIGNAL
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"
#  include "xsl/wheel.h"

#  include <cassert>
#  include <coroutine>
#  include <cstddef>
#  include <functional>
#  include <memory>
#  include <mutex>
#  include <span>
#  include <utility>
#  include <variant>

XSL_CORO_NB

struct SignalStorage {
  SignalStorage() : mtx(), state() {}
  std::mutex mtx;
  std::variant<std::size_t, std::function<void()>> state;
  bool stop = false;
};

template <class Storage>
struct SignalRxTraits;
template <>
struct SignalRxTraits<SignalStorage> {
  using storage_type = SignalStorage;
  /**
   * @brief Check if the signal is ready
   *
   * @param storage the signal storage
   * @return true if the signal is ready
   * @return false if the signal is not ready
   */
  static bool await_ready(storage_type &storage) {
    storage.mtx.lock();
    return storage.stop
           || (std::holds_alternative<std::size_t>(storage.state)
               && std::get<std::size_t>(storage.state) > 0);
  }
  /**
   * @brief Suspend the signal
   *
   * @tparam Promise the promise type
   * @param storage the signal storage
   * @param handle the coroutine handle
   */
  template <class Promise>
  static void await_suspend(storage_type &storage, std::coroutine_handle<Promise> handle) {
    storage.state = [handle] { handle.promise().resume(handle); };
    storage.mtx.unlock();
    DEBUG("Signal suspended");
  }
  /**
   * @brief Resume the signal
   *
   * @param storage the signal storage
   * @return true if the signal is still alive
   * @return false if the signal is not alive
   */
  [[nodiscard("must use the result of await_resume to confirm the signal is still alive")]]
  static bool await_resume(storage_type &storage) {
    Defer defer{[&storage] { storage.mtx.unlock(); }};
    auto &state = std::get<std::size_t>(storage.state);
    if (state > 0) {
      state--;
      DEBUG("Signal resumed {}", state);
      return true;
    }
    return false;
  }
};

/// @brief Signal receiver
template <class Pointer = std::shared_ptr<SignalStorage>>
class SignalReceiver {
public:
  using storage_type = SignalStorage;
  using traits_type = SignalRxTraits<storage_type>;

private:
  Pointer _storage;

public:
  template <class _Pointer>
  SignalReceiver(_Pointer &&storage) : _storage(std::forward<_Pointer>(storage)) {}
  SignalReceiver(SignalReceiver &&) = default;
  SignalReceiver &operator=(SignalReceiver &&) = default;
  /// @brief Check if the signal is ready
  bool await_ready() noexcept { return traits_type::await_ready(*this->_storage); }
  /// @brief Suspend the signal
  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) {
    return traits_type::await_suspend(*this->_storage, handle);
  }
  /// @brief Resume the signal
  [[nodiscard("must use the result of await_resume to confirm the signal is still alive")]]
  bool await_resume() {
    return traits_type::await_resume(*this->_storage);
  }
  /// @brief Pin the signal, return a raw SignalReceiver
  SignalReceiver<SignalStorage *> pin() { return {std::to_address(this->_storage)}; }
  /// @brief Check if the signal is valid
  operator bool() const { return this->_storage != nullptr; }
  /// @brief Check if the signal is disconnected
  bool is_disconnected() const {
    static_assert(std::is_same_v<Pointer, std::shared_ptr<SignalStorage>>);
    return this->_storage.use_count() == 1;
  }
};

template <class Storage, std::size_t MaxSignals>
struct SignalTxTraits;

template <std::size_t MaxSignals>
struct SignalTxTraits<SignalStorage, MaxSignals> {
  using storage_type = SignalStorage;
  /**
   * @brief Release the signal
   *
   * @param storage the signal storage
   */
  static void release(storage_type &storage) {
    storage.mtx.lock();
    if (std::size_t *state = std::get_if<std::size_t>(&storage.state); state != nullptr) {
      *state = [state] {
        if constexpr (MaxSignals > 1) {
          return std::min(*state + 1, MaxSignals);
        } else {
          return 1;
        }
      }();
      storage.mtx.unlock();
      DEBUG("Signal released {}", *state);
    } else {
      std::get<std::function<void()>>(std::exchange(storage.state, std::size_t{1}))();
      DEBUG("Signal Callback");
    }
  }
  /**
   * @brief Stop the signal
   *
   * @tparam Force if true, reset the signal count to 0
   * @param storage the signal storage
   * @return std::size_t the count of signals released
   */
  template <bool Force = false>
  static std::size_t stop(storage_type &storage) {
    storage.mtx.lock();
    storage.stop = true;
    std::size_t count = 0;
    if (std::size_t *state = std::get_if<std::size_t>(&storage.state); state != nullptr) {
      count = *state;
      if constexpr (Force) {
        *state = 0;
      }
      storage.mtx.unlock();
    } else {
      std::get<std::function<void()>>(std::exchange(storage.state, std::size_t{0}))();
    }
    return count;
  }
};

/// @brief Signal sender
template <std::size_t MaxSignals = std::dynamic_extent,
          class Pointer = std::shared_ptr<SignalStorage>>
class SignalSender {
public:
  using storage_type = SignalStorage;
  using traits_type = SignalTxTraits<storage_type, MaxSignals>;

private:
  Pointer _storage;

public:
  template <class _Pointer>
  SignalSender(_Pointer &&storage) : _storage(std::forward<_Pointer>(storage)) {}
  SignalSender(SignalSender &&) = default;
  SignalSender(const SignalSender &) = delete;
  SignalSender &operator=(SignalSender &&) = default;
  SignalSender &operator=(const SignalSender &) = delete;
  ~SignalSender() {
    if constexpr (std::is_pointer_v<Pointer>) {
      return;
    }
    if (this->_storage) {
      this->stop();
    }
  }
  /// @brief Release the signal
  void release() { return traits_type::release(*this->_storage); }
  /// @brief Stop the signal
  template <bool Force = false>
  std::size_t stop() {
    return traits_type::template stop<Force>(*this->_storage);
  }
  /// @brief Pin the signal, return a raw SignalSender
  SignalSender<MaxSignals, SignalStorage *> pin() { return {std::to_address(this->_storage)}; }
  /// @brief Check if the signal is valid
  operator bool() const { return this->_storage != nullptr; }
  /// @brief Check if the signal is disconnected
  bool is_disconnected() const {
    static_assert(std::is_same_v<Pointer, std::shared_ptr<SignalStorage>>);
    return this->_storage.use_count() == 1;
  }
};
/**
 * @brief Create a signal
 *
 * @tparam MaxSignals the maximum number of signals
 * @return decltype(auto) a pair of SignalReceiver and SignalSender
 */
template <std::size_t MaxSignals = std::dynamic_extent>
decltype(auto) signal() {
  auto storage = std::make_shared<SignalStorage>();
  auto copy = storage;
  return std::make_pair(SignalReceiver<decltype(storage)>(std::move(storage)),
                        SignalSender<MaxSignals, decltype(copy)>(std::move(copy)));
}
XSL_CORO_NE
#endif
