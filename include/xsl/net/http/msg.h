#pragma once
#ifndef XSL_NET_HTTP_MSG
#  define XSL_NET_HTTP_MSG
#  include "xsl/feature.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/net/transport.h"
HTTP_NB

using namespace transport::tcp;
class RequestView {
public:
  RequestView();
  ~RequestView();
  std::string_view method;
  std::string_view url;
  std::unordered_map<std::string_view, std::string_view> query;
  std::string_view version;
  std::unordered_map<std::string_view, std::string_view> headers;

  void clear();
};

class Request {
public:
  Request(std::string&& raw, RequestView view);
  ~Request();
  HttpMethod method;
  RequestView view;
  std::string raw;
};

class ResponseError {
public:
  ResponseError(int code, std::string_view message);
  ~ResponseError();
  int code;
  std::string_view message;
};

class IntoSendTasks {
public:
  virtual TcpSendTasks into_send_tasks() = 0;
  virtual ~IntoSendTasks() = default;
};

using IntoSendTasksPtr = std::unique_ptr<IntoSendTasks>;

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

class ResponsePart : public IntoSendTasks {
public:
  ResponsePart();
  ResponsePart(HttpVersion version, int status_code, std::string&& status_message);
  ~ResponsePart();
  int status_code;
  std::string status_message;
  HttpVersion version;
  std::unordered_map<std::string, std::string> headers;
  std::unique_ptr<TcpSendString<feature::node>> into_send_task_ptr();
  TcpSendTasks into_send_tasks();
  std::string to_string();
};

template <class B = void>
class HttpResponse;

template <std::derived_from<IntoSendTasks> B>
class HttpResponse<B> : public IntoSendTasks {
public:
  HttpResponse();
  HttpResponse(ResponsePart&& part);
  HttpResponse(ResponsePart&& part, B&& body);
  ~HttpResponse();
  ResponsePart part;
  std::optional<B> body;
  TcpSendTasks to_send_tasks();
};

template <std::derived_from<IntoSendTasks> B>
HttpResponse<B>::HttpResponse() {}
template <std::derived_from<IntoSendTasks> B>
HttpResponse<B>::HttpResponse(ResponsePart&& part) : part(std::move(part)), body(std::nullopt) {}
template <std::derived_from<IntoSendTasks> B>
HttpResponse<B>::HttpResponse(ResponsePart&& part, B&& body)
    : part(std::move(part)), body(make_optional(move(body))) {}
template <std::derived_from<IntoSendTasks> B>
HttpResponse<B>::~HttpResponse() {}
template <std::derived_from<IntoSendTasks> B>
TcpSendTasks HttpResponse<B>::to_send_tasks() {
  TcpSendTasks tasks;
  auto head = tasks.emplace_after(tasks.before_begin(), part.into_send_task_ptr());
  if (body) {
    tasks.emplace_after(head, body->into_send_tasks());
  }
  return tasks;
}

template <>
class HttpResponse<TcpSendTasks> : public IntoSendTasks {
public:
  HttpResponse();
  HttpResponse(ResponsePart&& part, TcpSendTasks&& tasks);
  ~HttpResponse();
  ResponsePart part;
  TcpSendTasks tasks;
  TcpSendTasks into_send_tasks();
};

template <>
class HttpResponse<std::string> : public IntoSendTasks {
public:
  HttpResponse();
  HttpResponse(ResponsePart&& part, std::string&& body);
  ~HttpResponse();
  ResponsePart part;
  std::string body;
  TcpSendTasks into_send_tasks();
};

using DefaultResponse = HttpResponse<std::string>;

// using ResponseResult = Result<Response, ResponseError>;

HTTP_NE
#endif
