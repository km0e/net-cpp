#pragma once

#ifndef _XSL_UTILS_WHEEL_RESULT_H_
#  define _XSL_UTILS_WHEEL_RESULT_H_
#  include "xsl/convert.h"
#  include "xsl/wheel/def.h"

#  include <optional>
#  include <stdexcept>
#  include <variant>

WHEEL_NAMESPACE_BEGIN

template <typename T, class E>
  requires ToString<E> || ToStringView<E>
class RefResult {
public:
  constexpr RefResult(const std::variant<T, E>& value) : value(value) {}
  constexpr bool is_ok() const { return std::holds_alternative<T>(value); }
  constexpr bool is_err() const { return std::holds_alternative<E>(value); }
  constexpr const T& unwrap() { return std::get<T>(value); }
  constexpr const E& unwrap_err() { return std::get<E>(value); }

private:
  const std::variant<T, E>& value;
};

template <typename E>
  requires ToString<E> || ToStringView<E>
class RefResult<void, E> {
public:
  constexpr RefResult(const std::optional<E>& value) : value(value) {}
  constexpr bool is_ok() const { return !value.has_value(); }
  constexpr bool is_err() const { return value.has_value(); }
  constexpr void unwrap() {
    if (is_err()) {
      if constexpr (ToString<E>) {
        throw std::runtime_error(format("unwrap error: {}", to_string(value.value())));
      } else {
        throw std::runtime_error(format("unwrap error: {}", to_string_view(value.value())));
      }
    }
  }
  constexpr const E& unwrap_err() { return value.value(); }

private:
  const std::optional<E>& value;
};

template <typename T, class E>
  requires ToString<E> || ToStringView<E>
class Result {
public:
  constexpr Result(T&& value) : value(std::forward<T>(value)) {}
  constexpr Result(E&& error) : value(std::forward<E>(error)) {}
  constexpr bool is_ok() const { return std::holds_alternative<T>(value); }
  constexpr bool is_err() const { return std::holds_alternative<E>(value); }
  constexpr T unwrap() {
    if (is_err()) {
      if constexpr (ToString<E>) {
        throw std::runtime_error(format("unwrap error: {}", to_string(std::get<E>(value))));
      } else {
        throw std::runtime_error(format("unwrap error: {}", to_string_view(std::get<E>(value))));
      }
    }
    return std::move(std::get<T>(value));
  }
  constexpr E unwrap_err() {
    if (is_ok()) {
      throw std::runtime_error("unwrap_err error");
    }
    return std::move(std::get<E>(value));
  }
  constexpr RefResult<T, E> as_ref() { return RefResult<T, E>(value); }

private:
  std::variant<T, E> value;
};

template <typename E>
  requires ToString<E> || ToStringView<E>
class Result<void, E> {
public:
  constexpr Result() : value() {}
  constexpr Result(E&& error) : value(std::forward<E>(error)) {}
  constexpr bool is_ok() const { return !value.has_value(); }
  constexpr bool is_err() const { return value.has_value(); }
  constexpr void unwrap() {
    if (is_err()) {
      if constexpr (ToString<E>) {
        throw std::runtime_error(format("unwrap error: {}", to_string(value.value())));
      } else {
        throw std::runtime_error(format("unwrap error: {}", to_string_view(value.value())));
      }
    }
  }
  constexpr E unwrap_err() {
    if (is_ok()) {
      throw std::runtime_error("unwrap_err error");
    }
    return std::move(value.value());
  }
  constexpr RefResult<void, E> as_ref() { return RefResult<void, E>(value); }

private:
  std::optional<E> value;
};

WHEEL_NAMESPACE_END
#endif
