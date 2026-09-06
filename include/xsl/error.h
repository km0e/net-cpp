/**
 * @file error.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Error handling utilities for the xsl library
 * @version 0.3.0
 * @date 2025-08-09
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_ERROR
#  define XSL_ERROR
#  include <xsl/def.h>
#  include <quill/DeferredFormatCodec.h>
#  include <xsl/log.h>
#  include <xsl/macro.h>
#  include <xsl/type_traits.h>

#  include <memory>
#  include <format>

XSL_NB

struct Error {
  constexpr Error(uint64_t ec) : _code(ec) {}
  constexpr Error(errc ec) : _code(static_cast<uint64_t>(ec)) {}
  constexpr Error(const Error&) = default;
  constexpr Error(Error&&) = default;
  constexpr Error& operator=(const Error&) = default;
  constexpr Error& operator=(Error&&) = default;
  virtual ~Error() = default;

  uint64_t code(this auto&& self) noexcept { return std::forward<decltype(self)>(self)._code; }

  virtual std::string_view message() const { return std::strerror(_code); }

  std::runtime_error to_exception() const {
    return std::runtime_error(std::format("Error {}: {}", _code, message()));
  }

private:
  int64_t _code;
};

struct StringError : public Error {
  StringError(uint64_t ec, const char* msg) : Error(ec), _msg(msg) {}
  StringError(uint64_t ec, std::string_view msg) : Error(ec), _msg(msg) {}
  StringError(uint64_t ec, std::string msg) : Error(ec), _msg(std::move(msg)) {}
  StringError(errc ec, std::string&& msg) : Error(ec), _msg(std::move(msg)) {}
  StringError(std::unique_ptr<Error>&& err, const char* msg)
      : Error(err->code()), _msg(std::format("{}{}{}", err->message(), "\n", msg)) {}
  StringError(const StringError&) = default;
  StringError(StringError&&) = default;
  StringError& operator=(const StringError&) = default;
  StringError& operator=(StringError&&) = default;
  ~StringError() override = default;

  std::string_view message() const override { return _msg; }

private:
  std::string _msg;
};

template <class T = void, class E = std::unique_ptr<xsl::Error>>
using Expected = std::expected<T, E>;

template <class T>
struct ResultJudge {
  constexpr bool operator!(this auto&& self) noexcept { return !self.value; }
};

template <>
struct ResultJudge<errc> {
  constexpr bool operator!(this auto&& self) noexcept { return self.value != errc{}; }
};

template <class T>
struct ResultJudge<T&> : ResultJudge<T> {};

template <class T, class RE, class CE = void>
struct ResultError;

/// @brief Specialization for unique_ptr<RE> error handling
template <class T, class E, class RE, class CE>
  requires std::is_base_of_v<RE, CE> && std::is_base_of_v<RE, E>
struct ResultError<Expected<T, std::unique_ptr<E>>, std::unique_ptr<RE>, CE> {
  inline std::unique_ptr<RE> operator()(this auto&& self)
    requires std::is_rvalue_reference_v<decltype(self)>
  {
    return std::move(self.value).error();
  }
  template <class Arg, class... Args>
    requires std::constructible_from<CE, std::unique_ptr<E>&&, Arg, Args...>
  inline std::unique_ptr<RE> operator()(this auto&& self, Arg&& arg, Args&&... args)
    requires std::is_rvalue_reference_v<decltype(self)>
  {
    return std::make_unique<CE>(std::move(self.value).error(), std::forward<Arg>(arg),
                                std::forward<Args>(args)...);
  }
};

template <class T, class E, class RE, class CE>
  requires std::is_base_of_v<RE, CE>
struct ResultError<Expected<T, E>, std::unique_ptr<RE>, CE> {
  inline std::unique_ptr<RE> operator()(this auto&& self)
    requires std::is_rvalue_reference_v<decltype(self)> && std::is_base_of_v<RE, E>
  {
    return std::make_unique<E>(std::move(self.value).error());
  }

  template <class... Args>
    requires std::constructible_from<CE, E&&, Args...>
  inline std::unique_ptr<RE> operator()(this auto&& self, Args&&... args)
    requires std::is_rvalue_reference_v<decltype(self)>
  {
    return std::make_unique<CE>(std::move(self.value).error(), std::forward<Args>(args)...);
  }
};

template <class T, class CE>
struct ResultError<Expected<T, errc>, std::unique_ptr<Error>, CE> {
  inline std::unique_ptr<Error> operator()(this auto&& self)
    requires std::is_rvalue_reference_v<decltype(self)>
  {
    return std::make_unique<Error>(std::move(self.value).error());
  }
  template <class... Args>
    requires std::constructible_from<CE, errc, Args...>
  inline std::unique_ptr<Error> operator()(this auto&& self, Args&&... args)
    requires std::is_rvalue_reference_v<decltype(self)>
  {
    return std::make_unique<CE>(std::move(self.value).error(), std::forward<Args>(args)...);
  }
};

template <class T>
struct ResultError<Expected<T, errc>, errc> {
  inline errc operator()(this auto&& self)
    requires std::is_rvalue_reference_v<decltype(self)>
  {
    return std::move(self.value).error();
  }
};

template <class RE, class CE>
  requires std::is_base_of_v<RE, CE>
struct ResultError<errc, std::unique_ptr<RE>, CE> {
  template <class... Args>
  inline std::unique_ptr<RE> operator()(this auto&& self, Args&&... args)
    requires std::is_rvalue_reference_v<decltype(self)>
  {
    return std::make_unique<CE>(std::move(self.value), std::forward<Args>(args)...);
  }
};

template <>
struct ResultError<errc, errc> {
  inline errc operator()(this auto&& self)
    requires std::is_rvalue_reference_v<decltype(self)>
  {
    return self.value;
  }
};

template <class T>
struct ResultError<T, errc> {
  inline errc operator()(this auto&& self)
    requires std::is_rvalue_reference_v<decltype(self)>
  {
    return errc{errno};
  }
  template <class Arg>
    requires std::constructible_from<errc, Arg>
  inline errc operator()(this auto&& self, Arg&& arg)
    requires std::is_rvalue_reference_v<decltype(self)>
  {
    return errc{std::forward<Arg>(arg)};
  }
};

template <class T, class RE, class CE>
  requires std::is_base_of_v<RE, CE>
struct ResultError<T, std::unique_ptr<RE>, CE> {
  template <class DE>
    requires std::is_base_of_v<RE, std::remove_reference_t<DE>>
  inline std::unique_ptr<RE> operator()(this auto&& self, DE&& err)
    requires std::is_rvalue_reference_v<decltype(self)>
  {
    return std::make_unique<std::remove_reference_t<DE>>(std::forward<DE>(err));
  }
  template <class... Args>
    requires std::constructible_from<CE, Args...>
  inline std::unique_ptr<RE> operator()(this auto&& self, Args&&... args)
    requires std::is_rvalue_reference_v<decltype(self)>
  {
    return std::make_unique<CE>(std::forward<Args>(args)...);
  }
};

template <class T, class RE, class CE>
  requires(sizeof(T) <= sizeof(void*))
struct ResultError<T&, std::unique_ptr<RE>, CE> : ResultError<T, std::unique_ptr<RE>, CE> {};

template <class T>
  requires(sizeof(T) <= sizeof(void*))
struct ResultError<T&, errc> : ResultError<T, errc> {};

template <class T>
struct ResultValue {
  inline auto operator*(this auto&& self) -> like_t<decltype(self), T> { return self.value; }
};

template <class T, class E>
struct ResultValue<Expected<T, E>> {
  inline auto operator*(this auto&& self) -> like_t<decltype(self), T> { return *self.value; }
};

template <class T, class E>
struct ResultValue<const Expected<T, E>&> {
  inline auto operator*(this auto&& self) -> like_t<decltype(self), const T> { return *self.value; }
};

template <class T, class RE, class CE>
struct ErrorUtils : ResultJudge<T>, ResultError<T, RE, CE>, ResultValue<T> {
  T value;
  template <class _T>
  constexpr ErrorUtils(_T&& v) : value(std::forward<_T>(v)) {}
};

template <class _T, class T = std::conditional_t<std::is_lvalue_reference_v<_T>, _T,
                                                 std::remove_reference_t<_T>>>
ErrorUtils<T, std::unique_ptr<Error>, StringError> make_result_utils(_T&& v) {
  return ErrorUtils<T, std::unique_ptr<Error>, StringError>(std::forward<_T>(v));
}

template <class _T, class T = std::conditional_t<std::is_lvalue_reference_v<_T>, _T,
                                                 std::remove_reference_t<_T>>>
ErrorUtils<T, errc, void> make_ec_utils(_T&& v) {
  return ErrorUtils<T, errc, void>(std::forward<_T>(v));
}

template <class E = Error, class E2 = StringError>
struct ErrorUtil {
  template <class... Args>
    requires std::constructible_from<E, Args...> && (!std::constructible_from<E2, Args...>)
  inline auto operator()(Args&&... args) const {
    return std::make_unique<E>(std::forward<Args>(args)...);
  }
  template <class... Args>
    requires std::constructible_from<E2, Args...>
  inline auto operator()(Args&&... args) const {
    return std::make_unique<E2>(std::forward<Args>(args)...);
  }
  template <class... Args>
    requires std::constructible_from<E2, Args...>
  inline auto operator()(bool, Args&&... args) const {
    return std::make_unique<E2>(std::forward<Args>(args)...);
  }
  template <class T>
  inline auto operator()(Expected<T>&& res) const {
    return std::move(res).error();
  }
  template <class T, class _E>
    requires std::is_base_of_v<E, _E>
  inline auto operator()(Expected<T, _E>&& res) const {
    return std::make_unique<E>(std::move(res).error());
  }
  template <class T>
  inline auto operator()(Expected<T, errc>&& res) const {
    return std::make_unique<E>(std::move(res).error());
  }
  template <class T>
  inline auto operator()(Expected<T, errc>&& res, std::invocable<errc> auto&& gen) const {
    return std::make_unique<E>(gen(std::move(res).error()));
  }

  template <class T, class _E>
  constexpr auto& a(Expected<T, _E>& res) {
    return *res;
  }
  constexpr auto& a(auto& res) { return res; }
};

struct EcUtil {
  inline auto operator()() const { return errc{errno}; }
  inline auto operator()(errc ec) const { return ec; }
  inline auto operator()(int ec) const { return errc{ec}; }
  template <class T>
  inline auto operator()(Expected<T, errc>&& res) const {
    return std::move(res).error();
  }

  template <class T, class _E>
  constexpr auto& a(Expected<T, _E>& res) {
    return *res;
  }
  constexpr auto& a(auto& res) { return res; }
};

inline std::string_view to_string_view(errc ec) { return std::strerror(static_cast<int>(ec)); }

#  define TRV(var, expr, ...) __BASE__TRV(, var, expr, xsl::make_result_utils, __VA_ARGS__)

#  define __BASE__TRV(co, var, expr, util, ...)                           \
    auto __result_##var = util(expr);                                     \
    if (!__result_##var) {                                                \
      co##return std::unexpected{std::move(__result_##var)(__VA_ARGS__)}; \
    }                                                                     \
    auto& var = *__result_##var;

#  define TRVEC(var, expr, ...) __BASE__TRV(, var, expr, xsl::make_ec_utils, __VA_ARGS__)

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
      auto __f = [&]<size_t I>(this auto&& self) -> xsl::Expected<void> {                  \
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

#  define __BASE__MUST1(expr, util)                \
    auto __result = util(expr);                    \
    if (!__result) {                               \
      throw std::move(__result)()->to_exception(); \
    }

#  define __MUST_1(expr) __BASE__MUST1(expr, xsl::make_result_utils)

#  define __BASE__MUST2(expr, util, var)                 \
    auto __result_##var = util(expr);                    \
    if (!__result_##var) {                               \
      throw std::move(__result_##var)()->to_exception(); \
    }                                                    \
    auto& var = *__result_##var;

#  define __MUST_2(expr, var) __BASE__MUST2(expr, xsl::make_result_utils, var)

/*
 * @brief Deal with the expected with graceful error handling
 * @example
 * ENSURE(expr) //
 * ENSURE(expr, err) //
 */
#  define ENSURE(...) __MACRO_DISPATCH(__ENSURE_, __VA_OPT__(, ) __VA_ARGS__)
#  define ENSURE2(...) __MACRO_DISPATCH(__ENSURE2_, __VA_OPT__(, ) __VA_ARGS__)

#  define ENSEC(expr, ...) __BASE__ENSURE(, expr, xsl::make_ec_utils, __VA_ARGS__)

#  define __BASE__ENSURE(co, expr, util, ...)                                \
    do {                                                                     \
      auto __res = util(expr);                                               \
      if (!__res) co##return std::unexpected{std::move(__res)(__VA_ARGS__)}; \
    } while (0)

#  define __BASE__ENSURE2(co, expr, util, ...)                                                   \
    do {                                                                                         \
      auto __res = (expr);                                                                       \
      if (!__res) co##return std::unexpected{util(std::move(__res) __VA_OPT__(, ) __VA_ARGS__)}; \
    } while (0)

#  define __ENSURE_2(co, expr) __BASE__ENSURE(co, expr, xsl::make_result_utils)
#  define __ENSURE_3(co, expr, ...) __BASE__ENSURE(co, expr, xsl::make_result_utils, __VA_ARGS__)
#  define __ENSURE_4(...) __ENSURE_3(__VA_ARGS__)
#  define __ENSURE_5(...) __ENSURE_3(__VA_ARGS__)

#  define __ENSURE2_2(co, expr, ...) \
    __BASE__ENSURE2(co, expr, xsl::ErrorUtil<> {} __VA_OPT__(, ) __VA_ARGS__)
#  define __ENSURE2_3(...) __ENSURE2_2(__VA_ARGS__)
#  define __ENSURE2_4(...) __ENSURE2_2(__VA_ARGS__)

#  define RETURN(...) \
    return std::unexpected { ResultError<void, std::unique_ptr<Error>, Error>{}(__VA_ARGS__) }

#  define CONTV(var, expr) __BASE__CONTV(var, expr, xsl::make_result_utils)

#  define __BASE__CONTV(var, expr, util) \
    auto __result_##var = util(expr);    \
    if (!__result_##var) {               \
      continue;                          \
    }                                    \
    auto& var = *__result_##var;

#  define CONT(expr) __BASE__CONT(expr)

#  define __BASE__CONT(expr)  \
    auto __result__ = (expr); \
    if (!__result__) {        \
      continue;               \
    }

XSL_NE

template <>
struct fmtquill::formatter<xsl::errc> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  auto format(xsl::errc const& user, format_context& ctx) const {
    return fmtquill::format_to(ctx.out(), "{}", std::strerror(static_cast<int>(user)));
  }
};
template <>
struct quill::Codec<xsl::errc> : quill::DeferredFormatCodec<xsl::errc> {};

#endif
