#include "xsl/convert.h"
#include "xsl/def.h"
#include "xsl/net/http/proto.h"

#include <system_error>
XSL_NAMESPACE_BEGIN
template <>
net::http::HttpVersion from_string_view<net::http::HttpVersion>(std::string_view version) {
  auto iter = std::ranges::find(net::http::HTTP_VERSION_STRINGS, version);
  if (iter == net::http::HTTP_VERSION_STRINGS.end()) return net::http::HttpVersion::UNKNOWN;
  return static_cast<net::http::HttpVersion>(iter - net::http::HTTP_VERSION_STRINGS.begin());
}
template <>
net::http::HttpMethod from_string_view<net::http::HttpMethod>(std::string_view method) {
  auto iter = std::ranges::find(net::http::HTTP_METHOD_STRINGS, method);
  if (iter == net::http::HTTP_METHOD_STRINGS.end()) return net::http::HttpMethod::UNKNOWN;
  return static_cast<net::http::HttpMethod>(iter - net::http::HTTP_METHOD_STRINGS.begin());
}
template <>
net::http::content_type::Type from_string_view<net::http::content_type::Type>(
    std::string_view type) {
  auto iter = std::ranges::find(net::http::content_type::TYPE_STRINGS, type);
  if (iter == net::http::content_type::TYPE_STRINGS.end())
    return net::http::content_type::Type::UNKNOWN;
  return static_cast<net::http::content_type::Type>(
      iter - net::http::content_type::TYPE_STRINGS.begin());
}
template <>
net::http::content_type::SubType from_string_view<net::http::content_type::SubType>(
    std::string_view subtype) {
  auto iter = std::ranges::find(net::http::content_type::SUB_TYPE_STRINGS, subtype);
  if (iter == net::http::content_type::SUB_TYPE_STRINGS.end())
    return net::http::content_type::SubType::UNKNOWN;
  return static_cast<net::http::content_type::SubType>(
      iter - net::http::content_type::SUB_TYPE_STRINGS.begin());
}
template <>
net::http::content_type::MediaType from_string_view(std::string_view type) {
  auto pos = type.find('/');
  if (pos == std::string_view::npos) return net::http::content_type::MediaType{};
  auto type_str = type.substr(0, pos);
  auto sub_type_str = type.substr(pos + 1);
  auto type_enum = from_string_view<net::http::content_type::Type>(type_str);
  auto sub_type_enum = from_string_view<net::http::content_type::SubType>(sub_type_str);
  return net::http::content_type::MediaType{type_enum, sub_type_enum};
}

std::string to_string(const std::error_code& ec) { return ec.message(); }

std::string to_string(const std::errc& ec) { return std::make_error_code(ec).message(); }

XSL_NAMESPACE_END
