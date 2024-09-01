/**
 * @file utils.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Utilities
 * @version 0.11
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_WHEEL_UTILS
#  define XSL_WHEEL_UTILS
#  include "xsl/wheel/def.h"
#  include "xsl/wheel/str.h"

#  include <exception>
#  include <functional>
#  include <iostream>
#  include <source_location>
#  include <string>
XSL_WHEEL_NB
namespace impl {

  class string_hasher : public std::hash<std::string>, public std::hash<std::string_view> {
  public:
    using is_transparent = void;
    using std::hash<std::string>::operator();
    using std::hash<std::string_view>::operator();
    auto operator()(const char* str) const { return this->operator()(std::string_view(str)); }
    auto operator()(const FixedString& str) const { return this->operator()(str.to_string_view()); }
  };
}  // namespace impl
/// @brief better string hash function
template <typename T>
using us_map = std::unordered_map<std::string, T, impl::string_hasher, std::equal_to<>>;
/**
 * @brief assert with message
 *
 * @param cond condition
 * @param msg message
 * @param loc location
 */
void rt_assert(bool cond, std::string_view msg,
               std::source_location loc = std::source_location::current());
/**
 * @brief assert with message
 *
 * @tparam Cond condition type
 * @tparam T message type
 * @param cond condition
 * @param msg message
 * @param loc location
 */
template <class Cond, class T>
void rt_assert(Cond&& cond, T msg, std::source_location loc = std::source_location::current()) {
  if (!cond) {
    std::println(std::cerr, "file: {}", loc.file_name());
    std::println(std::cerr, "line: {}", loc.line());
    std::println(std::cerr, "function: {}", loc.function_name());
    std::println(std::cerr, "Assertion failed: {}", msg);
    std::terminate();
  }
}

template <typename _Type, size_t _Extent>
  requires(!std::is_const_v<_Type>)
inline std::span<byte,
                 _Extent == std::dynamic_extent ? std::dynamic_extent : _Extent * sizeof(_Type)>
as_writable_bytes [[nodiscard]] (std::span<_Type, _Extent> __sp) noexcept {
  auto data = reinterpret_cast<byte*>(__sp.data());
  auto size = __sp.size_bytes();
  constexpr auto extent
      = _Extent == std::dynamic_extent ? std::dynamic_extent : _Extent * sizeof(_Type);
  return std::span<byte, extent>{data, size};
}

template <typename _Type, size_t _Extent>
[[nodiscard]]
inline std::span<const byte,
                 _Extent == std::dynamic_extent ? std::dynamic_extent : _Extent * sizeof(_Type)>
as_bytes(std::span<_Type, _Extent> __sp) noexcept {
  auto data = reinterpret_cast<const byte*>(__sp.data());
  auto size = __sp.size_bytes();
  constexpr auto extent
      = _Extent == std::dynamic_extent ? std::dynamic_extent : _Extent * sizeof(_Type);
  return std::span<const byte, extent>{data, size};
}
/// @brief Defer something to the end of the scope
template <class T>
class Defer {
public:
  Defer(T&& t) : _t(std::forward<T>(t)) {}
  ~Defer() { _t(); }

private:
  T _t;
};
XSL_WHEEL_NE
#endif
