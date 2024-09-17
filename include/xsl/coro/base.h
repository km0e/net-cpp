/**
 * @file base.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Base classes for coroutines
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_BASE
#  define XSL_CORO_BASE
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"

#  include <optional>
XSL_CORO_NB
/**
 * @brief Base class for coroutine promise
 *
 * @tparam ResultType
 */
template <class ResultType>
class PromiseBase {
public:
  using result_type = ResultType;

  constexpr PromiseBase() : _result(std::nullopt) {}

  constexpr auto get_return_object(this auto &&self) noexcept {
    LOG7("get_return_object");
    using promise_type = std::decay_t<decltype(self)>;
    using coro_type = promise_type::coro_type;
    return coro_type{std::coroutine_handle<promise_type>::from_promise(self)};
  }

  constexpr void unhandled_exception() { this->_result = std::unexpected{std::current_exception()}; }

  /**
   * @brief Return a value
   *
   * @return result_type
   */
  constexpr result_type operator*() {
    LOG7("PromiseBase operator*");
    if (*_result) {
      if constexpr (std::is_same_v<result_type, void>) {
        return **_result;
      } else {
        return std::move(**_result);
      }
    }
    std::rethrow_exception(_result->error());
  }

protected:
  std::optional<Result<result_type>> _result;
};

template <class Base>
class Promise : public Base {
protected:
  using Base::_result;

public:
  using typename Base::result_type;
  constexpr void return_value(result_type &&value) {
    LOG7("Promise return_value");
    _result = Result<result_type>(std::move(value));
  }
};

template <class Base>
  requires std::same_as<typename Base::result_type, void>
class Promise<Base> : public Base {
protected:
  using Base::_result;

public:
  using typename Base::result_type;
  constexpr void return_void() { _result = Result<void>(); }
};
XSL_CORO_NE
#endif
