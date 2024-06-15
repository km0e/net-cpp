#pragma once
#ifndef XSL_CORO_RESULT
#  define XSL_CORO_RESULT
#  include "xsl/coro/def.h"

#  include <exception>
#  include <variant>
XSL_CORO_NAMESPACE_BEGIN

template <typename T>
struct Result {
  explicit Result() = default;

  explicit Result(T &&value) : _value(value) {}

  explicit Result(std::exception_ptr &&exception_ptr) : _value(exception_ptr) {}

  T get_or_throw() {
    if (std::holds_alternative<std::exception_ptr>(_value)) {
      std::rethrow_exception(std::get<std::exception_ptr>(_value));
    }
    return std::move(std::get<T>(_value));
  }

private:
  std::variant<std::exception_ptr, T> _value{};
};

template <>
struct Result<void> {
  explicit Result() : _exception_ptr(nullptr) {}

  explicit Result(std::exception_ptr &&exception_ptr) : _exception_ptr(exception_ptr) {}

  void get_or_throw() {
    if (_exception_ptr) {
      std::rethrow_exception(_exception_ptr);
    }
  }

private:
  std::exception_ptr _exception_ptr;
};
XSL_CORO_NAMESPACE_END
#endif  // XSL_CORO_RESULT
