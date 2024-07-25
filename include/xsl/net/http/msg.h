#pragma once

#ifndef XSL_NET_HTTP_MSG
#  define XSL_NET_HTTP_MSG
#  include "xsl/coro/task.h"
#  include "xsl/net/http/body.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/proto.h"

#  include <concepts>
#  include <cstddef>
#  include <forward_list>
#  include <memory>
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

template <class Task>
concept SendTaskLike = requires(Task t) {
  {
    t(std::declval<sys::io::AsyncWriteDevice&>())
  } -> std::same_as<coro::Task<std::expected<void, sys::io::SendError>>>;
};

class IntoSendTask {
public:
  virtual coro::Task<std::expected<void, sys::io::SendError>> operator()(
      sys::io::AsyncWriteDevice& awd)
      = 0;
  virtual ~IntoSendTask() = default;
};

using IntoSendTaskPtr = std::unique_ptr<IntoSendTask>;

using SendTasks = std::forward_list<IntoSendTaskPtr>;

const int DEFAULT_HEADER_COUNT = 16;

const std::array<std::pair<std::string_view, std::string_view>, DEFAULT_HEADER_COUNT>
    DEFAULT_HEADERS = {
        std::pair{"Server", "XSL/0.1"},
        std::pair{"Content-Type", "text/plain"},
        std::pair{"Connection", "close"},
        std::pair{"Date", "Sun, 06 Nov 1994 08:49:37 GMT"},
        std::pair{"Last-Modified", "Sun, 06 Nov 1994 08:49:37 GMT"},
        std::pair{"Accept-Ranges", "bytes"},
        std::pair{"ETag", "\"359670651\""},
        std::pair{"Content-Length", "12345"},
        std::pair{"Cache-Control", "no-cache"},
        std::pair{"Expires", "Sun, 06 Nov 1994 08:49:37 GMT"},
        std::pair{"Pragma", "no-cache"},
        std::pair{"Content-Encoding", "gzip"},
        std::pair{"Vary", "Accept-Encoding"},
        std::pair{"X-Content-Type-Options", "nosniff"},
        std::pair{"X-Frame-Options", "DENY"},
        std::pair{"X-XSS-Protection", "1; mode=block"},
};

class ResponsePart : public IntoSendTask {
public:
  ResponsePart();
  ResponsePart(HttpVersion version, int status_code, std::string&& status_message);
  ~ResponsePart();
  int status_code;
  std::string status_message;
  HttpVersion version;
  std::unordered_map<std::string, std::string> headers;
  coro::Task<std::expected<void, sys::io::SendError>> operator()(sys::io::AsyncWriteDevice& awd);
  std::string to_string();
};

template <class B = void>
class HttpResponse;

template <SendTaskLike T>
class HttpResponse<T> : public IntoSendTask {
public:
  HttpResponse() {}
  HttpResponse(ResponsePart&& part) : part(std::move(part)), body(std::nullopt) {}
  HttpResponse(ResponsePart&& part, T&& body) : part(std::move(part)), body(std::move(body)) {}
  ~HttpResponse() {}
  ResponsePart part;
  T body;
  coro::Task<std::expected<void, sys::io::SendError>> operator()(sys::io::AsyncWriteDevice& awd) {
    co_await part(awd);
    co_await body(awd);
  }
};
template <>
class HttpResponse<std::string> : public IntoSendTask {
public:
  HttpResponse();
  HttpResponse(ResponsePart&& part, std::string&& body);
  ~HttpResponse();
  ResponsePart part;
  std::string body;
  coro::Task<std::expected<void, sys::io::SendError>> operator()(sys::io::AsyncWriteDevice& awd);
};

using DefaultResponse = HttpResponse<std::string>;

// using ResponseResult = Result<Response, ResponseError>;

HTTP_NE
#endif
