#pragma once
#ifndef _XSL_NET_HTTP_MSG_H_
#  define _XSL_NET_HTTP_MSG_H_
#  include "xsl/net/http/def.h"
#  include "xsl/net/transport.h"
#  include "xsl/wheel.h"
HTTP_NAMESPACE_BEGIN
using namespace transport::tcp;
class RequestView {
public:
  RequestView();
  ~RequestView();
  string_view method;
  string_view path;
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

// class Response {
// public:
//   virtual TcpSendTasks to_send_tasks() = 0;
// };

class ResponsePart : public IntoSendTasks {
public:
  ResponsePart();
  ResponsePart(int status_code, string&& status_message, HttpVersion version);
  ~ResponsePart();
  int status_code;
  string status_message;
  HttpVersion version;
  unordered_map<string, string> headers;
  unique_ptr<TcpSendString> into_send_task_ptr();
  TcpSendTasks into_send_tasks();
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
