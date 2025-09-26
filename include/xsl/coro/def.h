/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Definition of coroutines
 * @version 0.1.3
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_DEF
#  define XSL_CORO_DEF

#  define XSL_CORO_NB \
    XSL_NB            \
    namespace coro {

#  define XSL_CORO_NE \
    }                 \
    XSL_NE

#  include <xsl/macro.h>

#  include <coroutine>
#  include <utility>
XSL_CORO_NB

struct noop_coroutine {
  struct promise_type {
    using result_type = void;
    using has_context = std::bool_constant<false>;
    constexpr noop_coroutine get_return_object() { return noop_coroutine{}; }
    constexpr std::suspend_never initial_suspend() { return {}; }
    constexpr std::suspend_never final_suspend() noexcept { return {}; }
    constexpr void return_void() {}
    constexpr void unhandled_exception() {}
    template <class Promise>
    constexpr void resume(std::coroutine_handle<Promise>) {}
  };
  using promise_type = promise_type;
};

template <class Awaiter, class Coroutine = noop_coroutine>
concept Awaitable = requires() { [](Awaiter a) -> Coroutine { co_await std::move(a); }; };

template <class Awaiter>
class awaiter_traits {
public:
  using result_type = typename Awaiter::result_type;
};

XSL_CORO_NE

#endif  // XSL_CORO_DEF
