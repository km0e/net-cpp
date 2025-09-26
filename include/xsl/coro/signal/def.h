/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Common signal for coroutines
 * @version 0.2.0
 * @date 2024-09-14
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_SIGNAL_DEF
#  define XSL_CORO_SIGNAL_DEF
#  include <xsl/coro/core/context.h>
#  include <xsl/coro/def.h>

#  include <cassert>

XSL_CORO_NB

template <class Storage>
struct SignalAwaiterTraits;

struct SignalAwaiter {
  /// @brief Check if the signal is ready
  constexpr bool await_ready(this auto& self) noexcept { return self.get()->await_ready(); }
  /// @brief Suspend the signal
  template <class Promise>
  constexpr decltype(auto) await_suspend(this auto& self, std::coroutine_handle<Promise> handle) {
    return self.get()->await_suspend(handle);
  }
  /// @brief Resume the signal
  [[nodiscard("must use the result of await_resume to confirm the signal is still alive")]]
  constexpr decltype(auto) await_resume(this auto& self, coro::CoroContext& ctx) {
    return self.get()->await_resume(ctx);
  }
};

template <class Storage, std::ptrdiff_t MaxSignals>
struct SignalTraits;

/// @brief Signal sender
template <class Storage, std::ptrdiff_t MaxSignals>
class AnySignal : public SignalAwaiterTraits<Storage>, public SignalTraits<Storage, MaxSignals> {
public:
  using traits_type = SignalTraits<Storage, MaxSignals>;
  using storage_type = typename traits_type::storage_type;

private:
  friend struct SignalTraits<Storage, MaxSignals>;
  friend struct SignalAwaiterTraits<Storage>;
  Storage storage = {};

public:
  constexpr AnySignal() = default;
  template <class _Storage>
    requires(!std::same_as<std::remove_cvref_t<_Storage>, AnySignal>)
  constexpr AnySignal(_Storage&& storage) : storage(std::forward<decltype(storage)>(storage)) {}
  constexpr AnySignal(AnySignal&&) = default;
  constexpr AnySignal(const AnySignal&) = default;
  constexpr AnySignal& operator=(AnySignal&&) = default;
  constexpr AnySignal& operator=(const AnySignal&) = default;
  constexpr ~AnySignal() {}
};

XSL_CORO_NE
#endif
