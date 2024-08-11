#pragma once
#include <chrono>
#ifndef XSL_NET_HTTP_CONTEXT
#  define XSL_NET_HTTP_CONTEXT
#  include "xsl/ai/dev.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/proto.h"

#  include <optional>
XSL_HTTP_NB
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

  template <class Executor = coro::ExecutorBase>
  coro::Task<ai::Result, Executor> sendto(ByteWriter& awd) {
    if (!this->_response) {
      this->easy_resp(Status::INTERNAL_SERVER_ERROR);
    }
    this->check_and_add_date();
    return this->_response->sendto(awd);
  }

  std::string_view current_path;

  Request<ByteReader> request;

  std::optional<Response<ByteWriter>> _response;

private:
  void check_and_add_date() {
    if (!_response->_part.headers.contains("Date")) {
      _response->_part.headers.emplace("Date", to_date_string(std::chrono::system_clock::now()));
    }
  }
};

using HandleResult = coro::Task<void>;

template <class ByteReader, class ByteWriter>
using RouteHandler = std::function<HandleResult(HandleContext<ByteReader, ByteWriter>& ctx)>;

template <class ByteReader, class ByteWriter>
const RouteHandler<ByteReader, ByteWriter> UNKNOWN_HANDLER
    = []([[maybe_unused]] HandleContext<ByteReader, ByteWriter>& ctx) -> HandleResult {
  ctx.easy_resp(Status::UNKNOWN);
  co_return;
};

template <class ByteReader, class ByteWriter>
const RouteHandler<ByteReader, ByteWriter> INTERNAL_SERVER_ERROR_HANDLER
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
