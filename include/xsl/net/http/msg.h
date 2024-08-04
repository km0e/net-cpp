#pragma once

#ifndef XSL_NET_HTTP_MSG
#  define XSL_NET_HTTP_MSG
#  include "xsl/coro/executor.h"
#  include "xsl/coro/task.h"
#  include "xsl/feature.h"
#  include "xsl/net/http/body.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/net/io/buffer.h"
#  include "xsl/sys/net/io.h"

#  include <concepts>
#  include <cstddef>
#  include <functional>
#  include <optional>
#  include <string_view>
#  include <tuple>
#  include <utility>
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
  std::string to_string();
  void clear();
};

class Request {
public:
  Request(io::Buffer&& raw, RequestView&& view, BodyStream&& body)
      : method(xsl::from_string_view<HttpMethod>(view.method)),
        view(std::move(view)),
        raw(std::move(raw)),
        body(std::move(body)) {}

  Request(Request&&) = default;
  Request& operator=(Request&&) = default;
  ~Request() {}
  HttpMethod method;
  RequestView view;
  io::Buffer raw;

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
  ResponsePart(HttpVersion version, HttpStatus status_code, std::string_view&& status_message);
  ResponsePart(HttpVersion version, HttpStatus status_code);
  ResponsePart(HttpVersion version, uint16_t status_code);
  ResponsePart(ResponsePart&&) = default;
  ResponsePart& operator=(ResponsePart&&) = default;
  ~ResponsePart();
  HttpStatus status_code;
  std::string_view status_message;
  HttpVersion version;
  std::unordered_map<std::string, std::string> headers;
  std::string to_string();
};

class HttpResponse {
public:
  HttpResponse(ResponsePart&& part);
  template <std::invocable<sys::io::AsyncDevice<feature::Out<std::byte>>&> F>
  HttpResponse(ResponsePart&& part, F&& body)
      : _part(std::move(part)), _body(std::forward<F>(body)) {}
  template <std::convertible_to<std::string_view> T>
  HttpResponse(ResponsePart&& part, T&& body)
      : _part(std::move(part)),
        _body([body = std::forward<T>(body)](sys::io::AsyncDevice<feature::Out<std::byte>>& awd)
                  -> coro::Task<std::tuple<std::size_t, std::optional<std::errc>>> {
          return sys::net::immediate_send<coro::ExecutorBase>(awd, std::as_bytes(std::span(body)));
        }) {}
  HttpResponse(HttpResponse&&) = default;
  HttpResponse& operator=(HttpResponse&&) = default;
  ~HttpResponse();
  template <class Executor = coro::ExecutorBase>
  coro::Task<std::tuple<std::size_t, std::optional<std::errc>>, Executor> sendto(
      sys::io::AsyncDevice<feature::Out<std::byte>>& awd) {
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
      sys::io::AsyncDevice<feature::Out<std::byte>>&)>
      _body;
};
HTTP_NE
#endif
