#pragma once

#ifndef _XSL_UTILS_WHEEL_RESULT_H_
#  define _XSL_UTILS_WHEEL_RESULT_H_
#  include "xsl/wheel/def.h"
#  include "xsl/wheel/str.h"

#  include <variant>

WHEEL_NAMESPACE_BEGIN

template <typename T, ToString E>
class RefResult {
public:
  RefResult(const std::variant<T, E>& value) : value(value) {}
  constexpr bool is_ok() const { return std::holds_alternative<T>(value); }
  constexpr bool is_err() const { return std::holds_alternative<E>(value); }
  const T& unwrap() { return std::get<T>(value); }
  const E& unwrap_err() { return std::get<E>(value); }

private:
  const std::variant<T, E>& value;
};
template <typename T, ToString E>
class Result {
public:
  Result(T&& value) : value(std::forward<T>(value)) {}
  Result(E&& error) : value(std::forward<E>(error)) {}
  constexpr bool is_ok() const { return std::holds_alternative<T>(value); }
  constexpr bool is_err() const { return std::holds_alternative<E>(value); }
  T unwrap() {
    if (is_err()) {
      throw std::runtime_error(format("unwrap error: {}", to_string(std::get<E>(value))));
    }
    return std::move(std::get<T>(value));
  }
  E unwrap_err() {
    if (is_ok()) {
      throw std::runtime_error("unwrap_err error");
    }
    return std::move(std::get<E>(value));
  }
  RefResult<T, E> as_ref() { return RefResult<T, E>(value); }

private:
  std::variant<T, E> value;
};
WHEEL_NAMESPACE_END
#endif
