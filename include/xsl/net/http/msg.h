#pragma once
#ifndef _XSL_NET_HTTP_MSG_H_
#  define _XSL_NET_HTTP_MSG_H_
#  include "xsl/feature.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/net/transport.h"
#  include "xsl/wheel.h"
HTTP_NAMESPACE_BEGIN


using namespace transport::tcp;
class RequestView {
public:
  RequestView();
  ~RequestView();
  string_view method;
  string_view uri;
  string_view version;
  unordered_map<string_view, string_view> headers;
};

class Request {
public:
  Request(string&& raw, RequestView view);
  ~Request();
  HttpMethod method;
  RequestView view;
  string raw;
};

class ResponseError {
public:
  ResponseError(int code, string_view message);
  ~ResponseError();
  int code;
  string_view message;
};

class IntoSendTasks {
public:
  virtual TcpSendTasks into_send_tasks() = 0;
  virtual ~IntoSendTasks() = default;
};

using IntoSendTasksPtr = unique_ptr<IntoSendTasks>;

const int DEFAULT_HEADER_COUNT = 16;

const array<pair<string_view, string_view>, DEFAULT_HEADER_COUNT> DEFAULT_HEADERS = {
    pair{"Server", "XSL/0.1"},
    pair{"Content-Type", "text/plain"},
    pair{"Connection", "close"},
    pair{"Date", "Sun, 06 Nov 1994 08:49:37 GMT"},
    pair{"Last-Modified", "Sun, 06 Nov 1994 08:49:37 GMT"},
    pair{"Accept-Ranges", "bytes"},
    pair{"ETag", "\"359670651\""},
    pair{"Content-Length", "12345"},
    pair{"Cache-Control", "no-cache"},
    pair{"Expires", "Sun, 06 Nov 1994 08:49:37 GMT"},
    pair{"Pragma", "no-cache"},
    pair{"Content-Encoding", "gzip"},
    pair{"Vary", "Accept-Encoding"},
    pair{"X-Content-Type-Options", "nosniff"},
    pair{"X-Frame-Options", "DENY"},
    pair{"X-XSS-Protection", "1; mode=block"},
};

class ResponsePart : public IntoSendTasks {
public:
  ResponsePart();
  ResponsePart(int status_code, string&& status_message, HttpVersion version);
  ~ResponsePart();
  int status_code;
  string status_message;
  HttpVersion version;
  unordered_map<string, string> headers;
  unique_ptr<TcpSendString<feature::node>> into_send_task_ptr();
  TcpSendTasks into_send_tasks();
  string to_string();
};

template <class B>
class Response;

template <derived_from<IntoSendTasks> B>
class Response<B> : public IntoSendTasks {
public:
  Response();
  Response(ResponsePart&& part);
  Response(ResponsePart&& part, B&& body);
  ~Response();
  ResponsePart part;
  optional<B> body;
  TcpSendTasks to_send_tasks();
};

template <derived_from<IntoSendTasks> B>
Response<B>::Response() {}
template <derived_from<IntoSendTasks> B>
Response<B>::Response(ResponsePart&& part) : part(xsl::move(part)), body(nullopt) {}
template <derived_from<IntoSendTasks> B>
Response<B>::Response(ResponsePart&& part, B&& body)
    : part(xsl::move(part)), body(make_optional(move(body))) {}
template <derived_from<IntoSendTasks> B>
Response<B>::~Response() {}
template <derived_from<IntoSendTasks> B>
TcpSendTasks Response<B>::to_send_tasks() {
  TcpSendTasks tasks;
  auto head = tasks.emplace_after(tasks.before_begin(), part.into_send_task_ptr());
  if (body) {
    tasks.emplace_after(head, body->into_send_tasks());
  }
  return tasks;
}

template <>
class Response<TcpSendTasks> : public IntoSendTasks {
public:
  Response();
  Response(ResponsePart&& part, TcpSendTasks&& tasks);
  ~Response();
  ResponsePart part;
  TcpSendTasks tasks;
  TcpSendTasks into_send_tasks();
};

template <>
class Response<string> : public IntoSendTasks {
public:
  Response();
  Response(ResponsePart&& part, string&& body);
  ~Response();
  ResponsePart part;
  string body;
  TcpSendTasks into_send_tasks();
};

using DefaultResponse = Response<string>;

// using ResponseResult = Result<Response, ResponseError>;

HTTP_NAMESPACE_END
#endif
