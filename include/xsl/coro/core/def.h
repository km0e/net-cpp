/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Coroutine context
 * @version 0.1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_CORO_CORE_DEF
#  define XSL_CORO_CORE_DEF
#  include <xsl/coro/core/context.h>
#  include <xsl/coro/def.h>
#  include <xsl/wheel.h>

#  include <concepts>
#  include <variant>

XSL_CORO_NB

namespace _detail {
  template <class ResultType>
  using ResultTypeOrVoid
      = std::conditional_t<std::is_void_v<ResultType>,
                           std::variant<std::monostate, std::exception_ptr>,
                           std::variant<std::monostate, ResultType, std::exception_ptr>>;

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
}  // namespace _detail
template <class T>
concept PromiseType = requires {
  typename T::result_type;
  { T::has_context } -> std::convertible_to<bool>;
  { std::declval<T>().ctx() } -> std::same_as<Rc<CoroContext>&>;
};

XSL_CORO_NE
#endif
