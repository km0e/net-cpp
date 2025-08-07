/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Common signal for coroutines
 * @version 0.1.0
 * @date 2024-09-14
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_SIGNAL_DEF
#  define XSL_CORO_SIGNAL_DEF
#  include <xsl/coro/def.h>

#  include <cassert>

XSL_CORO_NB

template <class Storage>
struct SignalAwaiterTraits;

template <class Storage>
class SignalAwaiter : public SignalAwaiterTraits<Storage> {
  friend struct SignalAwaiterTraits<Storage>;

private:
  Storage &storage;

public:
  template <class _Storage>
    requires(!std::same_as<std::remove_cvref_t<_Storage>, SignalAwaiter>)
  explicit constexpr SignalAwaiter(_Storage &&storage) : storage(std::forward<_Storage>(storage)) {}
  constexpr SignalAwaiter(SignalAwaiter &&) = default;
  constexpr SignalAwaiter &operator=(SignalAwaiter &&) = default;
};

template <class Storage, std::ptrdiff_t MaxSignals>
struct SignalTraits;

/// @brief Signal sender
template <class Storage, std::ptrdiff_t MaxSignals>
class AnySignal : public SignalTraits<Storage, MaxSignals> {
public:
  using traits_type = SignalTraits<Storage, MaxSignals>;
  using storage_type = typename traits_type::storage_type;
  using awaiter_type = SignalAwaiter<storage_type>;

private:
  friend struct SignalTraits<Storage, MaxSignals>;
  Storage storage = {};

public:
  constexpr AnySignal() = default;
  template <class _Storage>
    requires(!std::same_as<std::remove_cvref_t<_Storage>, AnySignal>)
  constexpr AnySignal(_Storage &&storage) : storage(std::forward<decltype(storage)>(storage)) {}
  constexpr AnySignal(AnySignal &&) = default;
  constexpr AnySignal(const AnySignal &) = default;
  constexpr AnySignal &operator=(AnySignal &&) = default;
  constexpr AnySignal &operator=(const AnySignal &) = default;
  constexpr ~AnySignal() {}
  awaiter_type operator co_await() { return awaiter_type(storage); }
};

XSL_CORO_NE
#endif
