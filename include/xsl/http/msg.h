#pragma once
#ifndef _XSL_NET_HTTP_MSG_H_
#  define _XSL_NET_HTTP_MSG_H_
#  include "xsl/http/def.h"
#  include "xsl/transport/tcp/tcp.h"
#  include "xsl/wheel/wheel.h"
HTTP_NAMESPACE_BEGIN
using namespace transport::tcp;
class RequestView {
public:
  RequestView();
  ~RequestView();
  wheel::string_view method;
  wheel::string_view path;
  wheel::string_view version;
  wheel::unordered_map<wheel::string_view, wheel::string_view> headers;
};

class Request {
public:
  Request(wheel::string&& raw, RequestView view);
  ~Request();
  HttpMethod method;
  RequestView view;
  wheel::string raw;
};

class ResponseError {
public:
  ResponseError(int code, wheel::string_view message);
  ~ResponseError();
  int code;
  wheel::string_view message;
};

class IntoSendTasks {
public:
  virtual TcpSendTasks into_send_tasks() = 0;
  virtual ~IntoSendTasks() = default;
};

using IntoSendTasksPtr = wheel::unique_ptr<IntoSendTasks>;

// class Response {
// public:
//   virtual TcpSendTasks to_send_tasks() = 0;
// };

class ResponsePart : public IntoSendTasks {
public:
  ResponsePart();
  ResponsePart(int status_code, wheel::string&& status_message, HttpVersion version);
  ~ResponsePart();
  int status_code;
  wheel::string status_message;
  HttpVersion version;
  wheel::unordered_map<wheel::string, wheel::string> headers;
  wheel::unique_ptr<TcpSendString> into_send_task_ptr();
  TcpSendTasks into_send_tasks();
};

template <class B>
class Response;

template <wheel::derived_from<IntoSendTasks> B>
class Response<B> : public IntoSendTasks {
public:
  Response();
  Response(ResponsePart&& part);
  Response(ResponsePart&& part, B&& body);
  ~Response();
  ResponsePart part;
  wheel::optional<B> body;
  TcpSendTasks to_send_tasks();
};

template <wheel::derived_from<IntoSendTasks> B>
Response<B>::Response() {}
template <wheel::derived_from<IntoSendTasks> B>
Response<B>::Response(ResponsePart&& part) : part(wheel::move(part)), body(wheel::nullopt) {}
template <wheel::derived_from<IntoSendTasks> B>
Response<B>::Response(ResponsePart&& part, B&& body)
    : part(wheel::move(part)), body(wheel::make_optional(wheel::move(body))) {}
template <wheel::derived_from<IntoSendTasks> B>
Response<B>::~Response() {}
template <wheel::derived_from<IntoSendTasks> B>
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
class Response<wheel::string> : public IntoSendTasks {
public:
  Response();
  Response(ResponsePart&& part, wheel::string&& body);
  ~Response();
  ResponsePart part;
  wheel::string body;
  TcpSendTasks into_send_tasks();
};

using DefaultResponse = Response<wheel::string>;

// using ResponseResult = wheel::Result<Response, ResponseError>;

HTTP_NAMESPACE_END
#endif
