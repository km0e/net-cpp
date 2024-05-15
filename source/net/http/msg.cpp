#include "xsl/net/http/msg.h"
#include "xsl/wheel.h"
HTTP_NAMESPACE_BEGIN

RequestView::RequestView() : method(), path(), version(), headers() {}
RequestView::~RequestView() {}
Request::Request(string&& raw, RequestView view)
    : method(method_cast(view.method)), view(view), raw(xsl::move(raw)) {}
Request::~Request() {}
ResponseError::ResponseError(int code, string_view message) : code(code), message(message) {}
ResponseError::~ResponseError() {}

ResponsePart::ResponsePart()
    : status_code(0), status_message(), version(HttpVersion::UNKNOWN), headers() {}
ResponsePart::ResponsePart(int status_code, string&& status_message, HttpVersion version)
    : status_code(status_code), status_message(xsl::move(status_message)), version(version), headers() {}
ResponsePart::~ResponsePart() {}
unique_ptr<TcpSendString> ResponsePart::into_send_task_ptr() {
  string res;
  res.reserve(1024);
  res += version_cast(version);
  res += " ";
  res += to_string(status_code);
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
  return make_unique<TcpSendString>(xsl::move(res));
}
TcpSendTasks ResponsePart::into_send_tasks() {
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(), into_send_task_ptr());
  return tasks;
}
Response<TcpSendTasks>::Response() : part(), tasks() {}
Response<TcpSendTasks>::Response(ResponsePart&& part, TcpSendTasks&& tasks)
    : part(xsl::move(part)), tasks(xsl::move(tasks)) {}
Response<TcpSendTasks>::~Response() {}
TcpSendTasks Response<TcpSendTasks>::into_send_tasks() {
  this->tasks.emplace_after(this->tasks.before_begin(), this->part.into_send_task_ptr());
  return xsl::move(this->tasks);
}
Response<string>::Response() : part(), body() {}
Response<string>::Response(ResponsePart&& part, string&& body) : part(part), body(body) {}
Response<string>::~Response() {}
TcpSendTasks Response<string>::into_send_tasks() {
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(), make_unique<TcpSendString>(xsl::move(body)));
  tasks.emplace_after(tasks.before_begin(), part.into_send_task_ptr());
  return tasks;
}
HTTP_NAMESPACE_END
