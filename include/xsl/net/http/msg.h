#pragma once

#ifndef XSL_NET_HTTP_MSG
#  define XSL_NET_HTTP_MSG
#  include "xsl/ai/dev.h"
#  include "xsl/coro/executor.h"
#  include "xsl/coro/task.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/net/io/buffer.h"
#  include "xsl/wheel.h"

#  include <concepts>
#  include <cstddef>
#  include <functional>
#  include <optional>
#  include <string_view>
#  include <tuple>
#  include <utility>
XSL_HTTP_NB

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

template <ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
class Response {
public:
  template <class... Args>
  Response(ResponsePart&& part, Args&&... args)
      : _part(std::move(part)), _body(std::forward<Args>(args)...) {}
  Response(Response&&) = default;
  Response& operator=(Response&&) = default;
  ~Response() {}
  template <class Executor = coro::ExecutorBase>
  coro::Task<ai::Result, Executor> sendto(ByteWriter& awd) {
    auto str = this->_part.to_string();
    auto [sz, err] = co_await awd.template write<Executor>(std::as_bytes(std::span(str)));
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
  std::function<coro::Task<ai::Result>(ByteWriter&)> _body;
};

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

template <ai::AsyncReadDeviceLike<std::byte> ByteReader>
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
  Method method;
  RequestView view;
  io::Buffer<> raw;

  std::string_view content_part;
  ByteReader& _ard;
};

template <ai::AsyncReadDeviceLike<std::byte> ByteReader,
          ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
class HandleContext {
public:
  using response_body_type = coro::Task<ai::Result>(ByteWriter&);
  HandleContext(std::string_view current_path, Request<ByteReader>&& request)
      : current_path(current_path), request(std::move(request)), _response(std::nullopt) {}
  HandleContext(HandleContext&&) = default;
  HandleContext& operator=(HandleContext&&) = default;
  ~HandleContext() {}

  void easy_resp(Status status_code) {
    this->_response
        = Response<ByteWriter>{{Version::HTTP_1_1, status_code, to_reason_phrase(status_code)}};
  }

  template <std::invocable<ByteWriter&> F>
  void easy_resp(Status status_code, F&& body) {
    this->_response = Response<ByteWriter>{
        {Version::HTTP_1_1, status_code, to_reason_phrase(status_code)}, std::forward<F>(body)};
  }

  template <class... Args>
    requires std::constructible_from<std::string, Args...>
  void easy_resp(Status status_code, Args&&... args) {
    this->_response = Response<ByteWriter>{
        {Version::HTTP_1_1, status_code, to_reason_phrase(status_code)},
        [body = std::string(std::forward<Args>(args)...)](ByteWriter& awd)
            -> coro::Task<ai::Result> { return awd.write(std::as_bytes(std::span(body))); }};
  }

  void resp(ResponsePart&& part) { this->_response = Response<ByteWriter>{std::move(part)}; }

  template <std::invocable<ByteWriter&> F>
  void resp(ResponsePart&& part, F&& body) {
    this->_response = Response<ByteWriter>{{std::move(part)}, std::forward<F>(body)};
  }

  template <class... Args>
    requires std::constructible_from<std::string, Args...>
  void resp(ResponsePart&& part, Args&&... args) {
    this->_response = Response<ByteWriter>{{std::move(part)},
                                           [body = std::string(std::forward<Args>(args)...)](
                                               ByteWriter& awd) -> coro::Task<ai::Result> {
                                             return awd.write(std::as_bytes(std::span(body)));
                                           }};
  }

  std::string_view current_path;

  Request<ByteReader> request;

  std::optional<Response<ByteWriter>> _response;
};

enum class RouteError : uint8_t {
  Unknown,
  NotFound,
  Unimplemented,
};

const int ROUTE_ERROR_COUNT = 3;
const std::array<std::string_view, ROUTE_ERROR_COUNT> ROUTE_ERROR_STRINGS = {
    "Unknown",
    "NotFound",
    "Unimplemented",
};

inline std::string_view to_string_view(RouteError re) {
  return ROUTE_ERROR_STRINGS[static_cast<uint8_t>(re)];
}

using HandleResult = coro::Task<void>;

template <class ByteReader, class ByteWriter>
using RouteHandler = std::function<HandleResult(HandleContext<ByteReader, ByteWriter>& ctx)>;

template <class ByteReader, class ByteWriter>
const RouteHandler<ByteReader, ByteWriter> UNKNOWN_HANDLER
    = []([[maybe_unused]] HandleContext<ByteReader, ByteWriter>& ctx) -> HandleResult {
  ctx.easy_resp(Status::INTERNAL_SERVER_ERROR);
  co_return;
};

template <class ByteReader, class ByteWriter>
const RouteHandler<ByteReader, ByteWriter> NOT_FOUND_HANDLER
    = []([[maybe_unused]] HandleContext<ByteReader, ByteWriter>& ctx) -> HandleResult {
  ctx.easy_resp(Status::NOT_FOUND);
  co_return;
};

template <class ByteReader, class ByteWriter>
const RouteHandler<ByteReader, ByteWriter> NOT_IMPLEMENTED_HANDLER
    = []([[maybe_unused]] HandleContext<ByteReader, ByteWriter>& ctx) -> HandleResult {
  ctx.easy_resp(Status::NOT_IMPLEMENTED);
  co_return;
};

XSL_HTTP_NE
#endif
