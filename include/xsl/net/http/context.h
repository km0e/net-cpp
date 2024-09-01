/**
 * @file context.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP context
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_HTTP_CONTEXT
#  define XSL_NET_HTTP_CONTEXT
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/proto.h"

#  include <chrono>
#  include <optional>
XSL_HTTP_NB
using namespace xsl::io;
template <ABRL ByteReader, ABWL ByteWriter>
class HandleContext {
public:
  using response_body_type = Task<Result>(ByteWriter&);
  constexpr HandleContext(std::string_view current_path, Request<ByteReader>&& request)
      : current_path(current_path), request(std::move(request)), _response(std::nullopt) {}
  constexpr HandleContext(HandleContext&&) = default;
  constexpr HandleContext& operator=(HandleContext&&) = default;
  constexpr ~HandleContext() {}

  /// @brief easy response with status code
  constexpr void easy_resp(Status status_code) {
    this->_response
        = Response<ByteWriter>{{Version::HTTP_1_1, status_code, to_reason_phrase(status_code)}};
  }
  /// @brief easy response with status code and body
  constexpr void easy_resp(Status status_code, std::invocable<ByteWriter&> auto&& body) {
    this->_response
        = Response<ByteWriter>{{Version::HTTP_1_1, status_code, to_reason_phrase(status_code)},
                               std::forward<decltype(body)>(body)};
  }
  /// @brief easy response with status code and some arguments to construct the body
  template <class... Args>
    requires std::constructible_from<std::string, Args...>
  constexpr void easy_resp(Status status_code, Args&&... args) {
    this->_response = Response<ByteWriter>{
        {Version::HTTP_1_1, status_code, to_reason_phrase(status_code)},
        [body = std::string(std::forward<Args>(args)...)](ByteWriter& awd) -> Task<Result> {
          return awd.write(xsl::as_bytes(std::span(body)));
        }};
  }
  /// @brief response with ResponsePart
  constexpr void resp(ResponsePart&& part) { this->_response = Response<ByteWriter>{std::move(part)}; }
  /// @brief response with ResponsePart and body
  constexpr void resp(ResponsePart&& part, std::invocable<ByteWriter&> auto&& body) {
    this->_response = Response<ByteWriter>{{std::move(part)}, std::forward<decltype(body)>(body)};
  }
  /// @brief response with ResponsePart and some arguments to construct the body
  template <class... Args>
    requires std::constructible_from<std::string, Args...>
  constexpr void resp(ResponsePart&& part, Args&&... args) {
    this->_response = Response<ByteWriter>{
        {std::move(part)},
        [body = std::string(std::forward<Args>(args)...)](ByteWriter& awd) -> Task<Result> {
          return awd.write(xsl::as_bytes(std::span(body)));
        }};
  }
  /// @brief checkout the response
  constexpr Response<ByteWriter> checkout(this HandleContext self) {
    if (!self._response) {
      self.easy_resp(Status::INTERNAL_SERVER_ERROR);
    }
    self.check_and_add_date();
    return std::move(*self._response);
  }

  std::string_view current_path;

  Request<ByteReader> request;

  std::optional<Response<ByteWriter>> _response;

private:
  constexpr void check_and_add_date() {
    if (!_response->_part.headers.contains("Date")) {
      _response->_part.headers.emplace("Date", to_date_string(std::chrono::system_clock::now()));
    }
  }
};

using HandleResult = Task<std::optional<Status>>;

template <ABRL ByteReader, ABWL ByteWriter>
using Handler = std::function<HandleResult(HandleContext<ByteReader, ByteWriter>& ctx)>;

XSL_HTTP_NE
#endif
