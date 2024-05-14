#include "xsl/http/msg.h"
#include "xsl/transport/tcp/tcp.h"
#include "xsl/wheel/wheel.h"
HTTP_NAMESPACE_BEGIN

RequestView::RequestView() {}
RequestView::~RequestView() {}
Request::Request(wheel::string raw, RequestView view) : raw(raw), view(view) {
  method = method_cast(view.method);
}
Request::~Request() {}
ResponseError::ResponseError(int code, wheel::string_view message) : code(code), message(message) {}
ResponseError::~ResponseError() {}

ResponsePart::ResponsePart() {}
ResponsePart::ResponsePart(int status_code, wheel::string status_message, HttpVersion version)
    : status_code(status_code), status_message(status_message), version(version) {}
ResponsePart::~ResponsePart() {}
wheel::unique_ptr<TcpSendString> ResponsePart::into_send_task_ptr() {
  wheel::string res;
  res.reserve(1024);
  res += version_cast(version);
  res += " ";
  res += wheel::to_string(status_code);
  res += " ";
  res += status_message;
  res += "\r\n";
  for (const auto& [key, value] : headers) {
    res += key;
    res += ": ";
    res += value;
    res += "\r\n";
  }
  res += "\r\n";
  return wheel::make_unique<TcpSendString>(wheel::move(res));
}
TcpSendTasks ResponsePart::into_send_tasks() {
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(), into_send_task_ptr());
  return tasks;
}
Response<TcpSendTasks>::Response() {}
Response<TcpSendTasks>::Response(ResponsePart&& part,
                                              TcpSendTasks&& tasks)
    : part(wheel::move(part)), tasks(wheel::move(tasks)) {}
Response<TcpSendTasks>::~Response() {}
TcpSendTasks Response<TcpSendTasks>::into_send_tasks() {
  this->tasks.emplace_after(this->tasks.before_begin(), this->part.into_send_task_ptr());
  return wheel::move(this->tasks);
}
Response<wheel::string>::Response() {}
Response<wheel::string>::Response(ResponsePart&& part, wheel::string&& body)
    : part(part), body(body) {}
Response<wheel::string>::~Response() {}
TcpSendTasks Response<wheel::string>::into_send_tasks() {
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(),
                      wheel::make_unique<TcpSendString>(wheel::move(body)));
  tasks.emplace_after(tasks.before_begin(), part.into_send_task_ptr());
  return tasks;
}
HTTP_NAMESPACE_END
