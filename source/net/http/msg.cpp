#include "xsl/feature.h"
#include "xsl/net/http/msg.h"
#include "xsl/net/http/proto.h"
#include "xsl/wheel.h"

#include <spdlog/spdlog.h>

HTTP_NAMESPACE_BEGIN

RequestView::RequestView() : method(), uri(), version(), headers() {}
RequestView::~RequestView() {}
Request::Request(string&& raw, RequestView view)
    : method(wheel::from_string<HttpMethod>(view.method)), view(view), raw(xsl::move(raw)) {}
Request::~Request() {}
ResponseError::ResponseError(int code, string_view message) : code(code), message(message) {}
ResponseError::~ResponseError() {}

ResponsePart::ResponsePart()
    : status_code(0), status_message(), version(HttpVersion::UNKNOWN), headers() {}
ResponsePart::ResponsePart(int status_code, string&& status_message, HttpVersion version)
    : status_code(status_code),
      status_message(xsl::move(status_message)),
      version(version),
      headers() {}
ResponsePart::~ResponsePart() {}
unique_ptr<TcpSendString<feature::node>> ResponsePart::into_send_task_ptr() {
  return make_unique<TcpSendString<feature::node>>(this->to_string());
}
TcpSendTasks ResponsePart::into_send_tasks() {
  SPDLOG_DEBUG("ResponsePart::into_send_tasks");
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(), into_send_task_ptr());
  return tasks;
}
string ResponsePart::to_string() {
  string res;
  res.reserve(1024);
  res += http::to_string_view(version);
  res += " ";
  res += xsl::to_string(status_code);
  res += " ";
  res += status_message;
  res += "\r\n";
  for (const auto& [key, value] : headers) {
    res += key;
    res += ": ";
    res += value;
    res += "\r\n";
  }
  if (!headers.contains("Date")) {
    res += "Date: ";
    res += format("{:%a, %d %b %Y %T %Z}",
                  chrono::time_point_cast<chrono::seconds>(chrono::utc_clock::now()));
    res += "\r\n";
  }
  if (!headers.contains("Server")) {
    res += "Server: ";
    res += SERVER;
    res += "\r\n";
  }
  res += "\r\n";
  return res;
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
  tasks.emplace_after(tasks.before_begin(),
                      make_unique<TcpSendString<feature::node>>(xsl::move(body)));
  tasks.emplace_after(tasks.before_begin(), part.into_send_task_ptr());
  return tasks;
}
HTTP_NAMESPACE_END
