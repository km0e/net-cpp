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
#  include <exception>
#  include <utility>
#  include <variant>
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

namespace {
  template <class ResultType>
  using ResultTypeOrVoid
      = std::conditional_t<std::is_void_v<ResultType>,
                           std::variant<std::monostate, std::exception_ptr>,
                           std::variant<std::monostate, ResultType, std::exception_ptr>>;
}

template <class ResultType>
class Result : public ResultTypeOrVoid<ResultType> {
public:
  using base_type = ResultTypeOrVoid<ResultType>;
  using base_type::base_type;

  constexpr decltype(auto) unwrap(this Result&& self) {
    if (std::holds_alternative<std::exception_ptr>(self)) [[unlikely]] {
      std::rethrow_exception(std::get<std::exception_ptr>(std::move(self)));
    }
    if constexpr (std::is_same_v<ResultType, void>) {
      return;
    } else {
      return std::get<ResultType>(std::move(self));
    }
  }
};

#  define CO_TRV(...) __MACRO_DISPATCH(__CO_TRV_, __VA_ARGS__)

#  define __CO_TRV_2(expr, var) __BASE__TRV(co_, expr, var, std::move)

XSL_CORO_NE

#endif  // XSL_CORO_DEF
