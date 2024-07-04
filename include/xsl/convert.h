#pragma once
#ifndef _XSL_CONVERT_H_
#  define _XSL_CONVERT_H_
#  include "xsl/def.h"
#  include "xsl/net/http/proto.h"

#  include <string_view>
#  include <utility>
XSL_NAMESPACE_BEGIN

template <typename T>
T from_string_view(std::string_view str);

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

template <>
net::http::HttpVersion from_string_view(std::string_view type);

template <>
net::http::HttpMethod from_string_view(std::string_view type);

template <>
net::http::content_type::Type from_string_view(std::string_view type);

template <>
net::http::content_type::SubType from_string_view(std::string_view type);

template <>
net::http::content_type::MediaType from_string_view(std::string_view type);

XSL_NAMESPACE_END
#endif  // _XSL_CONVERT_H_
