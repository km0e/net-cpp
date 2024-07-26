#pragma once

#ifndef XSL_NET_HTTP_MSG
#  define XSL_NET_HTTP_MSG
#  include "xsl/coro/task.h"
#  include "xsl/net/http/body.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/proto.h"

#  include <concepts>
#  include <cstddef>
#  include <functional>
#  include <optional>
#  include <tuple>
HTTP_NB

class RequestView {
public:
  RequestView();
  RequestView(RequestView&&) = default;
  RequestView& operator=(RequestView&&) = default;
  ~RequestView();
  std::string_view method;
  std::string_view url;
  std::unordered_map<std::string_view, std::string_view> query;
  std::string_view version;
  std::unordered_map<std::string_view, std::string_view> headers;
  std::size_t length;
  std::string to_string();
  void clear();
};

class Request {
public:
  Request(std::string&& raw, RequestView&& view, BodyStream&& body);
  Request(Request&&) = default;
  Request& operator=(Request&&) = default;
  ~Request();
  HttpMethod method;
  RequestView view;
  std::string raw;

private:
  BodyStream body;
};

class ResponseError {
public:
  ResponseError(int code, std::string_view message);
  ~ResponseError();
  int code;
  std::string_view message;
};

const int DEFAULT_HEADER_COUNT = 16;

class ResponsePart {
public:
  ResponsePart();
  ResponsePart(HttpVersion version, int status_code, std::string&& status_message);
  ~ResponsePart();
  int status_code;
  std::string status_message;
  HttpVersion version;
  std::unordered_map<std::string, std::string> headers;
  std::string to_string();
};

class HttpResponse {
public:
  HttpResponse(ResponsePart&& part);
  template <std::invocable<sys::io::AsyncWriteDevice&> F>
  HttpResponse(ResponsePart&& part, F&& body)
      : part(std::move(part)), body(std::forward<F>(body)) {}
  HttpResponse(HttpResponse&&) = default;
  HttpResponse& operator=(HttpResponse&&) = default;
  ~HttpResponse();
  ResponsePart part;
  std::function<coro::Task<std::tuple<std::size_t, std::optional<std::errc>>>(
      sys::io::AsyncWriteDevice&)>
      body;
  coro::Task<std::tuple<std::size_t, std::optional<std::errc>>> sendto(
      sys::io::AsyncWriteDevice& awd);
};
HTTP_NE
#endif
