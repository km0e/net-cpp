/**
 * @file error.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Error handling utilities for the xsl library
 * @version 0.2.0
 * @date 2025-08-09
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_ERROR
#  define XSL_ERROR
#  include <xsl/def.h>
#  include <xsl/log.h>
#  include <xsl/macro.h>

#  include <memory>

XSL_NB

struct Error {
  constexpr Error(uint64_t ec) : _code(ec) {}
  constexpr Error(errc ec) : _code(static_cast<uint64_t>(ec)) {}
  constexpr Error(const Error &) = default;
  constexpr Error(Error &&) = default;
  constexpr Error &operator=(const Error &) = default;
  constexpr Error &operator=(Error &&) = default;
  virtual ~Error() = default;

  uint64_t code(this auto &&self) noexcept { return std::forward<decltype(self)>(self)._code; }

  virtual std::string_view message() const { return std::strerror(_code); }

  std::runtime_error to_exception() const {
    return std::runtime_error(std::format("Error {}: {}", _code, message()));
  }

private:
  int64_t _code;
};

struct StringError : public Error {
  StringError(uint64_t ec, const char *msg) : Error(ec), _msg(msg) {}
  StringError(uint64_t ec, std::string_view msg) : Error(ec), _msg(msg) {}
  StringError(uint64_t ec, std::string msg) : Error(ec), _msg(std::move(msg)) {}
  StringError(errc ec, std::string &&msg) : Error(ec), _msg(std::move(msg)) {}
  StringError(const StringError &) = default;
  StringError(StringError &&) = default;
  StringError &operator=(const StringError &) = default;
  StringError &operator=(StringError &&) = default;
  ~StringError() override = default;

  std::string_view message() const override { return _msg; }

private:
  std::string _msg;
};

template <class T = void, class E = std::unique_ptr<xsl::Error>>
using Expected = std::expected<T, E>;

template <class E = Error, class E2 = StringError>
struct ErrorUtil {
  template <class... Args>
    requires std::constructible_from<E, Args...> && (!std::constructible_from<E2, Args...>)
  inline auto operator()(Args &&...args) const {
    return std::make_unique<E>(std::forward<Args>(args)...);
  }
  template <class... Args>
    requires std::constructible_from<E2, Args...>
  inline auto operator()(Args &&...args) const {
    return std::make_unique<E2>(std::forward<Args>(args)...);
  }
  template <class... Args>
    requires std::constructible_from<E2, Args...>
  inline auto operator()(bool, Args &&...args) const {
    return std::make_unique<E2>(std::forward<Args>(args)...);
  }
  template <class T>
  inline auto operator()(Expected<T> &&res) const {
    return std::move(res).error();
  }
  template <class T, class _E>
    requires std::is_base_of_v<E, _E>
  inline auto operator()(Expected<T, _E> &&res) const {
    return std::make_unique<E>(std::move(res).error());
  }
  template <class T>
  inline auto operator()(Expected<T, errc> &&res) const {
    return std::make_unique<E>(std::move(res).error());
  }
  template <class T>
  inline auto operator()(Expected<T, errc> &&res, std::invocable<errc> auto &&gen) const {
    return std::make_unique<E>(gen(std::move(res).error()));
  }

  template <class T, class _E>
  constexpr auto &a(Expected<T, _E> &res) {
    return *res;
  }
  constexpr auto &a(auto &res) { return res; }
};

struct EcUtil {
  inline auto operator()() const { return errc{errno}; }
  inline auto operator()(errc ec) const { return ec; }
  inline auto operator()(int ec) const { return errc{ec}; }
  template <class T>
  inline auto operator()(Expected<T, errc> &&res) const {
    return std::move(res).error();
  }

  template <class T, class _E>
  constexpr auto &a(Expected<T, _E> &res) {
    return *res;
  }
  constexpr auto &a(auto &res) { return res; }
};

inline std::string_view to_string_view(errc ec) { return std::strerror(static_cast<int>(ec)); }

#  define TRV(var, expr, ...) __BASE__TRV(, var, expr, xsl::ErrorUtil<>{}, __VA_ARGS__)

#  define __BASE__TRV(co, var, expr, util, ...)                                               \
    auto __result_##var = (expr);                                                             \
    if (!__result_##var) {                                                                    \
      co##return std::unexpected{util(std::move(__result_##var) __VA_OPT__(, ) __VA_ARGS__)}; \
    }                                                                                         \
    auto &var = util.a(__result_##var);

#  define TRVEC(...) __MACRO_DISPATCH(__TRVEC_, __VA_ARGS__)

#  define __BASE__TRVEC_DIRECT(co, var, expr, util)                \
    auto __result_##var = (expr);                                  \
    if (!__result_##var) {                                         \
      co##return std::unexpected{util(std::move(__result_##var))}; \
    }                                                              \
    auto &var = util.a(__result_##var);

#  define __TRVEC_2(var, expr) __BASE__TRVEC_DIRECT(, var, expr, xsl::EcUtil{})

#  define EXPECT(...) __MACRO_DISPATCH(__EXPECT_, __VA_ARGS__)

#  define __BASE__EXPECT_3(co, expr, expected, util, gen)       \
    do {                                                        \
      auto e = (expr);                                          \
      if (e != expected) co##return std::unexpected{util(gen)}; \
    } while (0)

#  define EXPECT_N(expected, gen, ...)                                                     \
    do {                                                                                   \
      auto __gen = __WRAP_LAMBDA(gen);                                                     \
      auto __tup = std::make_tuple(__MAP_N(__WRAP_LAMBDA, __VA_ARGS__));                   \
      auto __f = [&]<size_t I>(this auto &&self) -> xsl::Expected<void> {                  \
        if constexpr (I >= std::tuple_size_v<decltype(__tup)>)                             \
          return {};                                                                       \
        else {                                                                             \
          __BASE__EXPECT_3(, std::get<I>(__tup)(), expected, xsl::ErrorUtil<>{}, __gen()); \
          return self.template operator()<I + 1>();                                        \
        }                                                                                  \
      };                                                                                   \
      ENSURE(__f.template operator()<0>());                                                \
    } while (0)

#  define MUST(...) __MACRO_DISPATCH(__MUST_, __VA_ARGS__)

#  define __BASE__MUST(expr, util)                             \
    do {                                                       \
      auto __result_##var = (expr);                            \
      if (!__result_##var) {                                   \
        throw util(std::move(__result_##var))->to_exception(); \
      }                                                        \
    } while (0)

#  define __BASE__MUST2(expr, util, var)                     \
    auto __result_##var = (expr);                            \
    if (!__result_##var) {                                   \
      throw util(std::move(__result_##var))->to_exception(); \
    }                                                        \
    auto &var = *__result_##var;

#  define __MUST_1(expr) __BASE__MUST(expr, xsl::ErrorUtil<>{})
#  define __MUST_2(expr, var) __BASE__MUST2(expr, xsl::ErrorUtil<>{}, var)

/*
 * @brief Deal with the expected with graceful error handling
 * @example
 * ENSURE(expr) //
 * ENSURE(expr, err) //
 */
#  define ENSURE(...) __MACRO_DISPATCH(__ENSURE_, __VA_OPT__(, ) __VA_ARGS__)
#  define ENSURE2(...) __MACRO_DISPATCH(__ENSURE2_, __VA_OPT__(, ) __VA_ARGS__)

#  define ENSEC(expr, ...) __BASE__ENSURE(, expr, xsl::EcUtil{}, __VA_ARGS__)

#  define __BASE__ENSURE(co, expr, util, ...)                    \
    do {                                                         \
      auto __res = static_cast<bool>(expr);                      \
      if (!__res) co##return std::unexpected{util(__VA_ARGS__)}; \
    } while (0)

#  define __BASE__ENSURE2(co, expr, util, ...)                                                   \
    do {                                                                                         \
      auto __res = (expr);                                                                       \
      if (!__res) co##return std::unexpected{util(std::move(__res) __VA_OPT__(, ) __VA_ARGS__)}; \
    } while (0)

#  define __ENSURE_2(co, expr) __BASE__ENSURE2(co, expr, xsl::ErrorUtil<>{})
#  define __ENSURE_3(co, expr, ...) __BASE__ENSURE(co, expr, xsl::ErrorUtil<>{}, __VA_ARGS__)
#  define __ENSURE_4(...) __ENSURE_3(__VA_ARGS__)

#  define __ENSURE2_2(co, expr, ...) \
    __BASE__ENSURE2(co, expr, xsl::ErrorUtil<> {} __VA_OPT__(, ) __VA_ARGS__)
#  define __ENSURE2_3(...) __ENSURE2_2(__VA_ARGS__)
#  define __ENSURE2_4(...) __ENSURE2_2(__VA_ARGS__)

#  define RETURN(...) \
    return std::unexpected { xsl::ErrorUtil<>{}(__VA_ARGS__) }

XSL_NE

#endif
