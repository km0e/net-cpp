/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Definition of coroutines
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_DEF
#  define XSL_CORO_DEF
#  define XSL_CORO_NB namespace xsl::_coro {
#  define XSL_CORO_NE }
#  include <coroutine>
#  include <exception>
#  include <expected>
#  include <utility>
XSL_CORO_NB

struct noop_coroutine {
  struct promise_type {
    using result_type = void;
    using executor_type = void;
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

template <class ResultType>
using Result = std::expected<ResultType, std::exception_ptr>;

XSL_CORO_NE

#endif  // XSL_CORO_DEF
