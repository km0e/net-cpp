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
#  include <string_view>
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
      : _part(std::move(part)), _body(std::forward<F>(body)) {}
  template <std::convertible_to<std::string_view> T>
  HttpResponse(ResponsePart&& part, T&& body)
      : _part(std::move(part)),
        _body([body = std::forward<T>(body)](sys::io::AsyncWriteDevice& awd)
                  -> coro::Task<std::tuple<std::size_t, std::optional<std::errc>>> {
          return sys::net::immediate_send<coro::ExecutorBase>(awd, std::as_bytes(std::span(body)));
        }) {}
  HttpResponse(HttpResponse&&) = default;
  HttpResponse& operator=(HttpResponse&&) = default;
  ~HttpResponse();
  template <class Executor = coro::ExecutorBase>
  coro::Task<std::tuple<std::size_t, std::optional<std::errc>>, Executor> sendto(
      sys::io::AsyncWriteDevice& awd) {
    auto str = this->_part.to_string();
    auto [sz, err]
        = co_await sys::net::immediate_send<Executor>(awd, std::as_bytes(std::span(str)));
    if (err) {
      co_return std::make_tuple(sz, err);
    };
    if (!_body) {
      co_return std::make_tuple(sz, std::nullopt);
    }
    auto [bsz, berr] = co_await this->_body(awd);
    if (berr) {
      co_return std::make_tuple(sz + bsz, berr);
    }
    co_return std::make_tuple(sz + bsz, std::nullopt);
  }
  ResponsePart _part;
  std::function<coro::Task<std::tuple<std::size_t, std::optional<std::errc>>>(
      sys::io::AsyncWriteDevice&)>
      _body;
};
HTTP_NE
#endif
