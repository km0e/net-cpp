/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Definitions and utilities for the xsl library
 * @version 0.2.0
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_DEF
#  define XSL_DEF
#  include <expected>
#  include <format>
#  include <functional>
#  include <system_error>

#  define XSL_NB namespace xsl {
#  define XSL_NE }

XSL_NB
using std::errc;

#  ifndef __cpp_lib_move_only_function
template <class F>
using move_only_function = std::function<F>;
#  else
using std::move_only_function;
#  endif  // __cpp_lib_move_only_function

using std::error_condition;
using std::expected;

template <class S>
class error_category : public std::error_category {
public:
  constexpr error_category(S msg) : _msg(std::move(msg)) {}
  constexpr error_category(const error_category &) = default;
  constexpr error_category(error_category &&) = default;
  constexpr error_category &operator=(const error_category &) = default;
  constexpr error_category &operator=(error_category &&) = default;
  const char *name() const noexcept override { return "xsl"; }
  std::string message(int ev) const override { return std::format("{}: {}", _msg, ev); }
  error_condition default_error_condition(int ev) const noexcept override {
    return error_condition(ev, *this);
  }

private:
  S _msg;
};

error_category(const char *msg) -> error_category<const char *>;
error_category(std::string &&msg) -> error_category<std::string>;

constexpr error_condition make_error_condition(errc ec, const char *msg) {
  return error_condition(static_cast<int>(ec), error_category(msg));
}

constexpr error_condition make_error_condition(errc ec, std::string &&msg) {
  return error_condition(static_cast<int>(ec), error_category(std::move(msg)));
}
constexpr auto make_ec_gen(const char *msg) {
  return [msg](errc ec) -> error_condition { return make_error_condition(ec, msg); };
}
constexpr auto make_ec_gen(std::string &&msg) {
  return [msg = std::move(msg)](errc ec) mutable -> error_condition {
    return make_error_condition(ec, std::move(msg));
  };
}
XSL_NE
#endif
