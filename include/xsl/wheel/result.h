#pragma once

#ifndef _XSL_UTILS_WHEEL_RESULT_H_
#  define _XSL_UTILS_WHEEL_RESULT_H_
#  include "xsl/wheel/def.h"
#  include "xsl/wheel/giant.h"

WHEEL_NAMESPACE_BEGIN

template <typename T>
concept ToString = requires(T t) {
  { to_string(t) } -> giant::convertible_to<giant::string_view>;
} || requires(T t) {
  { t.to_string() } -> giant::convertible_to<giant::string_view>;
};

template <ToString T>
giant::string to_string(const T& t) {
  if constexpr (requires { t.to_string(); }) {
    return t.to_string();
  } else {
    return to_string(t);
  }
}

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
