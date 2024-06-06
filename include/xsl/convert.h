#pragma once
#ifndef _XSL_CONVERT_H_
#  define _XSL_CONVERT_H_
#  include "xsl/def.h"
#  include "xsl/net/http/proto.h"

#  include <string_view>
XSL_NAMESPACE_BEGIN
template <typename T>
T from_string(std::string_view str);

template <>
net::http::HttpVersion from_string(std::string_view type);

template <>
net::http::HttpMethod from_string(std::string_view type);

template <>
net::http::content_type::Type from_string(std::string_view type);

template <>
net::http::content_type::SubType from_string(std::string_view type);

template <>
net::http::content_type::MediaType from_string(std::string_view type);

template <typename T>
concept ToString = requires(T t) {
  { to_string(t) } -> std::convertible_to<std::string_view>;
} || requires(T t) {
  { t.to_string() } -> std::convertible_to<std::string_view>;
};

template <typename T>
concept Stringable = requires(T t) {
  { t.to_string() } -> std::convertible_to<std::string_view>;
};

template <Stringable T>
std::string to_string(T t) {
  return t.to_string();
}

XSL_NAMESPACE_END
#endif  // _XSL_CONVERT_H_
