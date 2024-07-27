#pragma once

#ifndef XSL_NET_HTTP_MSG
#  define XSL_NET_HTTP_MSG
#  include "xsl/coro/executor.h"
#  include "xsl/coro/task.h"
#  include "xsl/net/http/body.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/sys/net/io.h"

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
  template <class Executor = coro::ExecutorBase>
  coro::Task<std::tuple<std::size_t, std::optional<std::errc>>, Executor> sendto(
      sys::io::AsyncWriteDevice& awd) {
    auto str = this->part.to_string();
    auto [sz, err]
        = co_await sys::net::immediate_send<Executor>(awd, std::as_bytes(std::span(str)));
    if (err) {
      co_return std::make_tuple(sz, err);
    };
    if (!body) {
      co_return std::make_tuple(sz, std::nullopt);
    }
    auto [bsz, berr] = co_await this->body(awd);
    if (berr) {
      co_return std::make_tuple(sz + bsz, berr);
    }
    co_return std::make_tuple(sz + bsz, std::nullopt);
  }
};
HTTP_NE
#endif
