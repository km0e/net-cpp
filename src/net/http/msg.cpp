#include "xsl/convert.h"
#include "xsl/feature.h"
#include "xsl/logctl.h"
#include "xsl/net/http/msg.h"
#include "xsl/net/http/proto.h"


HTTP_NAMESPACE_BEGIN

RequestView::RequestView() : method(), url(), query(), version(), headers() {}

RequestView::~RequestView() {}
void RequestView::clear() {
  method = std::string_view{};
  url = std::string_view{};
  query.clear();
  version = std::string_view{};
  headers.clear();
}

Request::Request(std::string&& raw, RequestView view)
    : method(xsl::from_string_view<HttpMethod>(view.method)), view(view), raw(std::move(raw)) {}
Request::~Request() {}
ResponseError::ResponseError(int code, std::string_view message) : code(code), message(message) {}
ResponseError::~ResponseError() {}

ResponsePart::ResponsePart()
    : status_code(0), status_message(), version(HttpVersion::UNKNOWN), headers() {}
ResponsePart::ResponsePart(HttpVersion version, int status_code, std::string&& status_message)
    : status_code(status_code),
      status_message(std::move(status_message)),
      version(version),
      headers() {}
ResponsePart::~ResponsePart() {}
std::unique_ptr<TcpSendString<feature::node>> ResponsePart::into_send_task_ptr() {
  return make_unique<TcpSendString<feature::node>>(this->to_string());
}
TcpSendTasks ResponsePart::into_send_tasks() {
  DEBUG("ResponsePart::into_send_tasks");
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(), into_send_task_ptr());
  return tasks;
}
std::string ResponsePart::to_string() {
  std::string res;
  res.reserve(1024);
  res += http::to_string_view(version);
  res += " ";
  res += std::to_string(status_code);
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
    res += format("{:%a, %d %b %Y %T %Z}", std::chrono::time_point_cast<std::chrono::seconds>(
                                               std::chrono::utc_clock::now()));
    res += "\r\n";
  }
  if (!headers.contains("Server")) {
    res += "Server: ";
    res += SERVER_VERSION;
    res += "\r\n";
  }
  res += "\r\n";
  return res;
}
HttpResponse<TcpSendTasks>::HttpResponse() : part(), tasks() {}
HttpResponse<TcpSendTasks>::HttpResponse(ResponsePart&& part, TcpSendTasks&& tasks)
    : part(std::move(part)), tasks(std::move(tasks)) {}
HttpResponse<TcpSendTasks>::~HttpResponse() {}
TcpSendTasks HttpResponse<TcpSendTasks>::into_send_tasks() {
  this->tasks.emplace_after(this->tasks.before_begin(), this->part.into_send_task_ptr());
  return std::move(this->tasks);
}
HttpResponse<std::string>::HttpResponse() : part(), body() {}
HttpResponse<std::string>::HttpResponse(ResponsePart&& part, std::string&& body)
    : part(part), body(body) {}
HttpResponse<std::string>::~HttpResponse() {}
TcpSendTasks HttpResponse<std::string>::into_send_tasks() {
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(),
                      make_unique<TcpSendString<feature::node>>(std::move(body)));
  tasks.emplace_after(tasks.before_begin(), part.into_send_task_ptr());
  return tasks;
}
HTTP_NAMESPACE_END
