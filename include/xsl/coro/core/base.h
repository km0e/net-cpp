/**
 * @file base.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Base classes for coroutines
 * @version 0.1.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_BASE
#  define XSL_CORO_BASE
#  include <xsl/coro/def.h>
#  include <xsl/log.h>

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

  constexpr PromiseBase() : _result() {}

  constexpr auto get_return_object(this auto &&self) noexcept {
    log_trace("get_return_object");
    using promise_type = std::decay_t<decltype(self)>;
    using coro_type = promise_type::coro_type;
    return coro_type{std::coroutine_handle<promise_type>::from_promise(self)};
  }

  constexpr void unhandled_exception() { this->_result = std::current_exception(); }

  /**
   * @brief Return a value
   *
   * @return result_type
   */
  constexpr result_type operator*() {
    log_trace("PromiseBase operator*");
    return std::move(_result).unwrap();
  }

protected:
  Result<result_type> _result;
};

template <class Base>
class Promise : public Base {
protected:
  using Base::_result;

public:
  using typename Base::result_type;
  constexpr void return_value(result_type &&value) {
    log_trace("Promise return_value");
    _result.template emplace<result_type>(std::move(value));
  }
};

template <class Base>
  requires std::same_as<typename Base::result_type, void>
class Promise<Base> : public Base {
protected:
  using Base::_result;

public:
  using typename Base::result_type;
  constexpr void return_void() {}
};
XSL_CORO_NE
#endif
