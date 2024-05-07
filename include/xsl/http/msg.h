#pragma once
#ifndef _XSL_NET_HTTP_MSG_H_
#  define _XSL_NET_HTTP_MSG_H_
#  include <xsl/http/http.h>
#  include <xsl/utils/wheel/wheel.h>
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
enum class RequestErrorKind {
  Unknown,
  InvalidFormat,
};

class RequestError {
public:
  RequestError(RequestErrorKind kind);
  RequestError(RequestErrorKind kind, wheel::string message);
  ~RequestError();
  RequestErrorKind kind;
  wheel::string message;
};

class Request {
public:
  Request();
  ~Request();
  wheel::string raw;
  wheel::string_view method_view;
  HttpMethod method;
  wheel::string_view path;
  wheel::string_view version;
  wheel::unordered_map<wheel::string_view, wheel::string_view> headers;
  wheel::string_view body;
};
using RequestResult = wheel::Result<Request, RequestError>;

class ResponseError {
public:
  ResponseError(int code, wheel::string_view message);
  ~ResponseError();
  int code;
  wheel::string_view message;
};

struct Response {
public:
  Response();
  Response(wheel::string version, int status_code, wheel::string status_message);
  ~Response();
  wheel::string version;
  int status_code;
  wheel::string status_message;
  wheel::unordered_map<wheel::string, wheel::string> headers;
  wheel::string body;
  wheel::string to_string() const;
};
using ResponseResult = wheel::Result<Response, ResponseError>;

HTTP_NAMESPACE_END
#endif
