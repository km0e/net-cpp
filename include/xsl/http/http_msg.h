#pragma once
#ifndef _XSL_NET_HTTP_MSG_H_
#  define _XSL_NET_HTTP_MSG_H_
#  include <xsl/http/config.h>
#  include <xsl/utils/wheel/wheel.h>
HTTP_NAMESPACE_BEGIN
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

class HttpRequest {
public:
  HttpRequest();
  ~HttpRequest();
  wheel::string raw;
  wheel::string_view method;
  wheel::string_view path;
  wheel::string_view version;
  wheel::unordered_map<wheel::string_view, wheel::string_view> headers;
  wheel::string_view body;
};
using RequestResult = wheel::Result<HttpRequest, RequestError>;

class ResponseError {
public:
  ResponseError(int code, wheel::string_view message);
  ~ResponseError();
  int code;
  wheel::string_view message;
};

class HttpResponse {
public:
  HttpResponse();
  ~HttpResponse();
  wheel::string version;
  int status_code;
  wheel::string status_message;
  wheel::unordered_map<wheel::string, wheel::string> headers;
  wheel::string body;
  wheel::string to_string() const;
};
using ResponseResult = wheel::Result<HttpResponse, ResponseError>;

HTTP_NAMESPACE_END
#endif
