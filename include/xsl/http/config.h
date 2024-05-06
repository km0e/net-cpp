#pragma once
#ifndef _XSL_NET_HTTP_CONFIG_H_
#  define _XSL_NET_HTTP_CONFIG_H_
#  include <xsl/utils/wheel/wheel.h>

#  define HTTP_NAMESPACE_BEGIN namespace xsl::http {
#  define HTTP_NAMESPACE_END }
HTTP_NAMESPACE_BEGIN
enum class HttpMethod {
  GET,
  POST,
  PUT,
  DELETE,
  PATCH,
  HEAD,
  OPTIONS,
  TRACE,
  CONNECT,
};

class HttpError {
public:
  HttpError(int code, wheel::string_view message);
  ~HttpError();
  int code;
  wheel::string_view message;
};
HTTP_NAMESPACE_END
#endif
