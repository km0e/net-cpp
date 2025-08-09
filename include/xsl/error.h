/**
 * @file error.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Error handling utilities for the xsl library
 * @version 0.1.0
 * @date 2025-08-09
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_ERROR
#  define XSL_ERROR
#  include <xsl/def.h>
#  include <xsl/macro.h>

#  include <cstring>
#  include <variant>
XSL_NB
class Error {
public:
  constexpr Error(int ec, const char *msg) : ec(ec), msg(msg) {}
  constexpr Error(int ec, std::string &&msg) : ec(ec), msg(std::move(msg)) {}

  constexpr Error() : ec(errno), msg(std::strerror(errno)) {}
  constexpr Error(int ec) : Error(ec, "") {}
  constexpr Error(errc ec) : Error(static_cast<int>(ec), std::strerror(static_cast<int>(ec))) {}

  constexpr Error(errc ec, const char *msg) : Error(static_cast<int>(ec), msg) {}
  constexpr Error(errc ec, std::string &&msg) : Error(static_cast<int>(ec), std::move(msg)) {}

  constexpr Error(const Error &) = default;
  constexpr Error(Error &&) = default;
  constexpr Error &operator=(const Error &) = default;
  constexpr Error &operator=(Error &&) = default;

  constexpr const char *message() const {
    if (std::holds_alternative<const char *>(msg)) {
      return std::get<const char *>(msg);
    } else {
      return std::get<std::string>(msg).c_str();
    }
  }
  constexpr void message(std::string &out) const {
    if (std::holds_alternative<const char *>(msg)) {
      out = std::get<const char *>(msg);
    } else {
      out = std::get<std::string>(msg);
    }
  }
  constexpr int code() const { return ec; }

private:
  int ec;
  std::variant<const char *, std::string> msg;
};
template <class T>
using Expected = std::expected<T, Error>;

/*
 * @brief Deal with the expected with graceful error handling
 * @param ... variadic arguments
 * @example
 * TRY(expr) // expr must return std::expected<T, E>
 * TRY(expr, var) // expr must return std::expected<T, E>, var will be assigned the value of T
 */
#  define TRY(...) __MACRO_DISPATCH(__TRY_, __VA_ARGS__)

#  define __BASE__TRY(co, var, expr, ret) \
    auto var = (expr);                    \
    if (!var) co##return ret;

#  define __TRY_2(var, expr) __BASE__TRY(, var, expr, )
#  define __TRY_3(var, expr, ec) __BASE__TRY(, var, expr, std::unexpected{Error(ec)})
#  define __TRY_4(var, expr, ec, msg) __BASE__TRY(, var, expr, std::unexpected{Error(ec, msg)})
#  define __TRY_5(var, expr, ec, ...) \
    __BASE__TRY(, var, expr, std::unexpected{Error(ec, std::format(__VA_ARGS__))})
#  define __TRY_6(...) __TRY_5(__VA_ARGS__)

#  define TRV(...) __MACRO_DISPATCH(__TRV_, __VA_ARGS__)

#  define __BASE__TRV(co, var, expr)                      \
    auto __result_##var = (expr);                         \
    if (!__result_##var) {                                \
      co##return std::unexpected{__result_##var.error()}; \
    }                                                     \
    auto &var = *__result_##var;

#  define __TRV_2(var, expr) __BASE__TRV(, var, expr)
// #  define __TRV_3(var, expr, gen) __BASE__TRV(, var, expr, gen)

/*
 * @brief Deal with the expected with graceful error handling
 * @example
 * ENSURE(expr) //
 * ENSURE(expr, err) //
 */
#  define ENSURE(...) __MACRO_DISPATCH(__ENSURE_, __VA_ARGS__)

#  define __BASE__ENSURE(co, expr, ret) \
    do {                                \
      if (!(expr)) co##return ret;      \
    } while (0)

#  define __ENSURE_1(expr) __BASE__ENSURE(, expr, )
#  define __ENSURE_2(expr, ec) __BASE__ENSURE(, expr, std::unexpected{Error(ec)})
#  define __ENSURE_3(expr, ec, msg) __BASE__ENSURE(, expr, std::unexpected{Error(ec, msg)})
#  define __ENSURE_4(expr, ec, ...) \
    __BASE__ENSURE(, expr, std::unexpected{Error(ec, std::format(__VA_ARGS__))})

#  define HNSURE(...) __MACRO_DISPATCH(__HNSURE_, __VA_ARGS__)

#  define __BASE__HNSURE(co, expr, gen)       \
    do {                                      \
      auto __result = (expr);                 \
      if (!__result) {                        \
        auto e = std::move(__result.error()); \
        co##return std::unexpected{gen};      \
      }                                       \
    } while (0)

#  define __HNSURE_1(expr) __BASE__HNSURE(, expr, std::move(e))
#  define __HNSURE_2(expr, gen) __BASE__HNSURE(, expr, gen)

#  define EXPECT(...) __MACRO_DISPATCH(__EXPECT_, __VA_ARGS__)

#  define __BASE__EXPECT_3(co, expr, expected, gen)       \
    do {                                                  \
      auto e = (expr);                                    \
      if (e != expected) co##return std::unexpected{gen}; \
    } while (0)

#  define __EXPECT_3(expr, expected, gen) __BASE__EXPECT_3(, expr, expected, gen)

#  define EXPECT_N(expected, gen, e1, e2, ...)  \
    do {                                        \
      auto __list = {e1, e2, __VA_ARGS__};      \
      for (auto &__e : __list) {                \
        __BASE__EXPECT_3(, __e, expected, gen); \
      }                                         \
    } while (0)

#  define MUST(...) __MACRO_DISPATCH(__MUST_, __VA_ARGS__)

#  define __BASE__MUST(expr)                                        \
    do {                                                            \
      auto __result_##var = (expr);                                 \
      if (!__result_##var) {                                        \
        throw std::runtime_error(__result_##var.error().message()); \
      }                                                             \
    } while (0)

#  define __BASE__MUST2(expr, var)                                \
    auto __result_##var = (expr);                                 \
    if (!__result_##var) {                                        \
      throw std::runtime_error(__result_##var.error().message()); \
    }                                                             \
    auto &var = *__result_##var;

#  define __MUST_1(expr) __BASE__MUST(expr)
#  define __MUST_2(expr, var) __BASE__MUST2(expr, var)
XSL_NE
#endif
