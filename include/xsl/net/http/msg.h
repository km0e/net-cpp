/**
 * @file msg.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-25
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#ifndef XSL_NET_HTTP_MSG
#  define XSL_NET_HTTP_MSG
#  include "xsl/coro.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/net/io/buffer.h"
#  include "xsl/wheel.h"

#  include <functional>
#  include <optional>
#  include <string_view>
#  include <tuple>
#  include <utility>
XSL_HTTP_NB
using namespace xsl::io;

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
template <ABWL ByteWriter>
class Response {
public:
  template <class... Args>
  Response(ResponsePart&& part, Args&&... args)
      : _part(std::move(part)), _body(std::forward<Args>(args)...) {}
  Response(Response&&) = default;
  Response& operator=(Response&&) = default;
  ~Response() {}

  Task<Result> sendto(ByteWriter& awd) {
    auto str = this->_part.to_string();
    DEBUG("response: {}", str);
    auto [sz, err] = co_await awd.write(xsl::as_bytes(std::span(str)));
    if (err) {
      co_return std::make_tuple(sz, err);
    };
    if (!_body) {
      co_return std::make_tuple(sz, std::nullopt);
    }
    auto [bodySize, bodyError] = co_await this->_body(awd);
    if (bodyError) {
      co_return std::make_tuple(sz + bodySize, bodyError);
    }
    co_return std::make_tuple(sz + bodySize, std::nullopt);
  }
  ResponsePart _part;
  std::function<Task<Result>(ByteWriter&)> _body;
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
  std::string to_string();

  void clear();
};
/// @brief the request
template <ABRL ByteReader>
class Request {
public:
  Request(io::Buffer<>&& raw, RequestView&& view, std::string_view content_part, ByteReader& ard)
      : method(xsl::from_string_view<Method>(view.method)),
        view(std::move(view)),
        raw(std::move(raw)),
        content_part(content_part),
        _ard(ard) {}

  Request(Request&&) = default;
  Request& operator=(Request&&) = default;
  ~Request() {}

  /// @brief check if the request has the header
  [[nodiscard]]
  inline bool has_header(std::string_view key) {
    return this->view.headers.find(key) != this->view.headers.end();
  }
  /// @brief get the header
  [[nodiscard]]
  inline std::optional<std::string_view> get_header(std::string_view key) {
    auto iter = this->view.headers.find(key);
    if (iter == this->view.headers.end()) {
      return std::nullopt;
    }
    return iter->second;
  }

  Method method;
  RequestView view;
  io::Buffer<> raw;

  std::string_view content_part;
  ByteReader& _ard;
};

XSL_HTTP_NE
#endif
