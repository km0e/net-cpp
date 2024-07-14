#pragma once
#ifndef XSL_CONVERT
#  define XSL_CONVERT
#  include "xsl/def.h"
#  include <string_view>
#  include <system_error>
#  include <utility>
XSL_NB

template <typename T>
T from_string_view(std::string_view str);

// for std::error_code
std::string to_string(const std::error_code& ec);

// for std::errc
std::string to_string(const std::errc& ec);

template <typename T>
concept ToString = std::convertible_to<T, std::string> || requires(T t) {
  { to_string(t) } -> std::convertible_to<std::string>;
} || requires(T t) {
  { t.to_string() } -> std::convertible_to<std::string>;
};

template <class T>
  requires requires(T t) {
    { t.to_string() } -> std::convertible_to<std::string>;
  }
std::string to_string(T&& t) {
  return t.to_string();
}

template <class T>
  requires std::convertible_to<T, std::string>
std::string to_string(T&& t) {
  return std::forward<T>(t);
}

template <class T>
concept ToStringView = std::convertible_to<T, std::string_view> || requires(T t) {
  { to_string_view(t) } -> std::convertible_to<std::string_view>;
} || requires(T t) {
  { t.to_string_view() } -> std::convertible_to<std::string_view>;
};

template <class T>
  requires requires(T t) {
    { t.to_string_view() } -> std::convertible_to<std::string_view>;
  }
std::string_view to_string_view(T&& t) {
  return t.to_string_view();
}

template <class T>
  requires std::convertible_to<T, std::string_view>
std::string_view to_string_view(T&& t) {
  return std::forward<T>(t);
}

XSL_NE
#endif  // XSL_CONVERT
