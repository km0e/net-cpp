/**
 * @file common.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Common signal for coroutines
 * @version 0.1
 * @date 2024-09-14
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_SIGNAL_COMMON
#  define XSL_CORO_SIGNAL_COMMON
#  include "xsl/coro/def.h"

#  include <memory>
XSL_CORO_NB

template <class Storage>
struct SignalRxTraits;

template <class Traits, class Pointer>
class SignalAwaiter {
public:
  using traits_type = Traits;
  using storage_type = typename traits_type::storage_type;

private:
  Pointer _storage;

public:
  template <class _Storage>
    requires(!std::same_as<std::remove_cvref_t<_Storage>, SignalAwaiter>)
  explicit constexpr SignalAwaiter(_Storage &&storage)
      : _storage(std::forward<_Storage>(storage)) {}
  constexpr SignalAwaiter(SignalAwaiter &&) = default;
  constexpr SignalAwaiter &operator=(SignalAwaiter &&) = default;
  /// @brief Check if the signal is ready
  constexpr bool await_ready() noexcept { return traits_type::ready(*this->_storage); }
  /// @brief Suspend the signal
  template <class Promise>
  constexpr decltype(auto) await_suspend(std::coroutine_handle<Promise> handle) {
    return traits_type::suspend(*this->_storage, handle);
  }
  /// @brief Resume the signal
  [[nodiscard("must use the result of await_resume to confirm the signal is still alive")]]
  constexpr bool await_resume() {
    return traits_type::resume(*this->_storage);
  }
};

template <class Storage, std::ptrdiff_t MaxSignals>
struct SignalTxTraits;

/// @brief Signal sender
template <class TxTraits, class Pointer>
class AnySignal {
public:
  using traits_type = TxTraits;
  using storage_type = typename traits_type::storage_type;
  using awaiter_type = typename traits_type::awaiter_type;
  using max_signals = traits_type::max_signals;
  using pointer_traits = std::pointer_traits<Pointer>;

private:
  Pointer _storage;

public:
  constexpr AnySignal() : _storage(std::make_shared<storage_type>()) {}
  template <class _Storage>
    requires(!std::same_as<std::remove_cvref_t<_Storage>, AnySignal>)
  constexpr AnySignal(_Storage &&storage) : _storage(std::forward<decltype(storage)>(storage)) {}
  constexpr AnySignal(AnySignal &&) = default;
  constexpr AnySignal(const AnySignal &) = default;
  constexpr AnySignal &operator=(AnySignal &&) = default;
  constexpr AnySignal &operator=(const AnySignal &) = default;
  constexpr ~AnySignal() {
    if constexpr (std::is_pointer_v<Pointer>) {
      return;
    }
    if (this->_storage) {
      this->stop();
    }
  }
  awaiter_type operator co_await() const {
    assert(this->_storage != nullptr && "Signal has been moved");
    return awaiter_type{std::to_address(this->_storage)};
  }
  /// @brief Release the signal
  constexpr bool release() {
    assert(this->_storage != nullptr && "Signal has been moved");
    return traits_type::release(*this->_storage);
  }
  /// @brief Stop the signal
  constexpr bool stop() {
    assert(this->_storage != nullptr && "Signal has been moved");
    return traits_type::stop(*this->_storage);
  }
  constexpr std::optional<std::ptrdiff_t> force_stop() {
    assert(this->_storage != nullptr && "Signal has been moved");
    return traits_type::force_stop(*this->_storage);
  }
  /// @brief Pin the signal, return a raw SignalSender
  constexpr AnySignal<traits_type, storage_type *> pin() {
    assert(this->_storage != nullptr && "Signal has been moved");
    return {std::to_address(this->_storage)};
  }
  /// @brief Check if the signal is valid
  constexpr operator bool() const { return this->_storage != nullptr; }
  /// @brief Check if the signal is disconnected
  constexpr std::ptrdiff_t use_count() const {
    static_assert(std::is_same_v<Pointer, std::shared_ptr<storage_type>>);
    return this->_storage.use_count();
  }
};

XSL_CORO_NE
#endif
