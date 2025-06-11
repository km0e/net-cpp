/**
 * @file msg.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP message definitions
 * @version 0.1
 * @date 2024-08-25
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#ifndef XSL_ASIO_HTTP_MSG
#  define XSL_ASIO_HTTP_MSG
#  include "xsl/asio/http/def.h"
#  include "xsl/coro.h"
#  include "xsl/io/byte.h"
#  include "xsl/io/def.h"
#  include "xsl/net.h"
#  include "xsl/wheel.h"

#  include <functional>
#  include <optional>
#  include <string_view>
#  include <utility>
XSL_ASIO_HTTP_NB
using namespace xsl::io;
using namespace xsl::http;

const int DEFAULT_HEADER_COUNT = 16;
/// @brief the response part
class ResponsePart {
public:
  ResponsePart();
  ResponsePart(Version version, Status status_code, std::string_view&& status_message);
  ResponsePart(Version version, Status status_code);
  ResponsePart(Version version, uint16_t status_code);
  ResponsePart(Status status_code);
  ResponsePart(uint16_t status_code);
  ResponsePart(ResponsePart&&) = default;
  ResponsePart& operator=(ResponsePart&&) = default;
  ~ResponsePart();
  Status status_code;
  std::string_view status_message;
  Version version;
  us_map<std::string> headers;
  std::string to_string();
};
/// @brief the response
template <AsyncWrite W>
class Response {  // TODO: abstract the body
public:
  constexpr Response(ResponsePart&& part, auto&&... args)
      : _part(std::move(part)), _body(std::forward<decltype(args)>(args)...) {}
  constexpr Response(Response&&) = default;
  constexpr Response& operator=(Response&&) = default;
  ~Response() {}

  Task<Result> sendto(W& awd) {
    auto str = this->_part.to_string();
    log_trace("response: {}", str);
    auto [sz, err] = co_await awd.write(std::as_bytes(std::span(str)));
    if (err) {
      co_return {sz};
    };
    if (!_body) {
      co_return {sz};
    }
    auto [bodySize, bodyError] = co_await this->_body(awd);
    if (bodyError) {
      co_return {sz + bodySize, bodyError};
    }
    co_return {sz + bodySize};
  }
  ResponsePart _part;
  std::function<Task<Result>(W&)> _body;
};
/// @brief the request view
class RequestView {
public:
  RequestView();
  RequestView(RequestView&&) = default;
  RequestView& operator=(RequestView&&) = default;
  ~RequestView();
  std::string_view method;

  std::string_view scheme;
  std::string_view authority;
  std::string_view path;
  std::unordered_map<std::string_view, std::string_view> query;

  std::string_view version;
  std::unordered_map<std::string_view, std::string_view> headers;
  constexpr std::string to_string();

  constexpr void clear() {
    method = std::string_view{};
    query.clear();
    version = std::string_view{};
    headers.clear();
  }
};
/// @brief the request
template <AsyncRead R>
class Request {  // TODO: abstract the ard
public:
  constexpr Request(ByteBuffer&& raw, RequestView&& view, std::string_view content_part, R& aid)
      : method(Method::from_string_view(view.method)),
        view(std::move(view)),
        raw(std::move(raw)),
        content_part(content_part),
        _ard(aid) {}

  constexpr Request(Request&&) = default;
  constexpr Request& operator=(Request&&) = default;
  ~Request() {}

  /// @brief check if the request has the header
  [[nodiscard]]
  constexpr bool has_header(std::string_view key) {
    return this->view.headers.find(key) != this->view.headers.end();
  }
  /// @brief get the header
  [[nodiscard]]
  constexpr std::optional<std::string_view> get_header(std::string_view key) {
    auto iter = this->view.headers.find(key);
    if (iter == this->view.headers.end()) {
      return std::nullopt;
    }
    return iter->second;
  }

  Method method;
  RequestView view;
  ByteBuffer raw;

  std::string_view content_part;
  R& _ard;
};

XSL_ASIO_HTTP_NE
#endif
