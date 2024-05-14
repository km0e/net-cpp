#pragma once
#ifndef _XSL_NET_HTTP_DEF_H_
#  define _XSL_NET_HTTP_DEF_H_
#  include "xsl/wheel/wheel.h"

#  include <cstdint>
#  define HTTP_NAMESPACE_BEGIN namespace xsl::net::http::detail {
#  define HTTP_NAMESPACE_END }
HTTP_NAMESPACE_BEGIN
enum class HttpMethod : uint8_t {
  EXT,
  GET,
  POST,
  PUT,
  DELETE,
  HEAD,
  OPTIONS,
  TRACE,
  CONNECT,
  UNKNOWN = 0xff,
};
const int METHOD_COUNT = 9;
wheel::string method_cast(HttpMethod method);
HttpMethod method_cast(wheel::string_view method);

enum class HttpVersion : uint8_t {
  EXT,
  HTTP_1_0,
  HTTP_1_1,
  HTTP_2_0,
  UNKNOWN = 0xff,
};

wheel::string version_cast(HttpVersion method);
HttpVersion version_cast(wheel::string_view method);

HTTP_NAMESPACE_END
#endif
