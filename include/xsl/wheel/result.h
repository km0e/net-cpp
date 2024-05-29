#pragma once

#ifndef _XSL_UTILS_WHEEL_RESULT_H_
#  define _XSL_UTILS_WHEEL_RESULT_H_
#  include "xsl/wheel/def.h"
#  include "xsl/wheel/giant.h"
#  include "xsl/wheel/str.h"

WHEEL_NAMESPACE_BEGIN


template <typename T, ToString E>
class RefResult {
public:
  RefResult(const giant::variant<T, E>& value) : value(value) {}
  constexpr bool is_ok() const { return giant::holds_alternative<T>(value); }
  constexpr bool is_err() const { return giant::holds_alternative<E>(value); }
  const T& unwrap() { return giant::get<T>(value); }
  const E& unwrap_err() { return giant::get<E>(value); }

private:
  const giant::variant<T, E>& value;
};
template <typename T, ToString E>
class Result {
public:
  Result(T&& value) : value(giant::forward<T>(value)) {}
  Result(E&& error) : value(giant::forward<E>(error)) {}
  constexpr bool is_ok() const { return giant::holds_alternative<T>(value); }
  constexpr bool is_err() const { return giant::holds_alternative<E>(value); }
  T unwrap() {
    if (is_err()) {
      throw giant::runtime_error(format("unwrap error: {}", to_string(giant::get<E>(value))));
    }
    return giant::move(giant::get<T>(value));
  }
  E unwrap_err() {
    if (is_ok()) {
      throw giant::runtime_error("unwrap_err error");
    }
    return giant::move(giant::get<E>(value));
  }
  RefResult<T, E> as_ref() { return RefResult<T, E>(value); }

private:
  giant::variant<T, E> value;
};
WHEEL_NAMESPACE_END
#endif
