#include "xsl/net/http/msg.h"
#include "xsl/wheel.h"
HTTP_NAMESPACE_BEGIN
string method_cast(HttpMethod method) {
  switch (method) {
    case HttpMethod::EXT:
      return "EXT";
    case HttpMethod::GET:
      return "GET";
    case HttpMethod::POST:
      return "POST";
    case HttpMethod::PUT:
      return "PUT";
    case HttpMethod::DELETE:
      return "DELETE";
    case HttpMethod::HEAD:
      return "HEAD";
    case HttpMethod::OPTIONS:
      return "OPTIONS";
    case HttpMethod::TRACE:
      return "TRACE";
    case HttpMethod::CONNECT:
      return "CONNECT";
    default:
      return "Unknown";
  }
}
HttpMethod method_cast(string_view method) {
  if (method == "EXT") {
    return HttpMethod::EXT;
  } else if (method == "GET") {
    return HttpMethod::GET;
  } else if (method == "POST") {
    return HttpMethod::POST;
  } else if (method == "PUT") {
    return HttpMethod::PUT;
  } else if (method == "DELETE") {
    return HttpMethod::DELETE;
  } else if (method == "HEAD") {
    return HttpMethod::HEAD;
  } else if (method == "OPTIONS") {
    return HttpMethod::OPTIONS;
  } else if (method == "TRACE") {
    return HttpMethod::TRACE;
  } else if (method == "CONNECT") {
    return HttpMethod::CONNECT;
  } else {
    return HttpMethod::UNKNOWN;
  }
}
string http_version_cast(HttpVersion version) {
  switch (version) {
    case HttpVersion::EXT:
      return "EXT";
    case HttpVersion::HTTP_1_0:
      return "HTTP/1.0";
    case HttpVersion::HTTP_1_1:
      return "HTTP/1.1";
    case HttpVersion::HTTP_2_0:
      return "HTTP/2.0";
    default:
      return "Unknown";
  }
}
HttpVersion http_version_cast(string_view version) {
  if (version == "EXT") {
    return HttpVersion::EXT;
  } else if (version == "HTTP/1.0") {
    return HttpVersion::HTTP_1_0;
  } else if (version == "HTTP/1.1") {
    return HttpVersion::HTTP_1_1;
  } else if (version == "HTTP/2.0") {
    return HttpVersion::HTTP_2_0;
  } else {
    return HttpVersion::UNKNOWN;
  }
}
RequestView::RequestView() : method(), uri(), version(), headers() {}
RequestView::~RequestView() {}
Request::Request(string&& raw, RequestView view)
    : method(method_cast(view.method)), view(view), raw(xsl::move(raw)) {}
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
unique_ptr<TcpSendString> ResponsePart::into_send_task_ptr() {
  return make_unique<TcpSendString>(xsl::move(this->to_string()));
}
TcpSendTasks ResponsePart::into_send_tasks() {
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(), into_send_task_ptr());
  return tasks;
}
string ResponsePart::to_string() {
  string res;
  res.reserve(1024);
  res += http_version_cast(version);
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
  tasks.emplace_after(tasks.before_begin(), make_unique<TcpSendString>(xsl::move(body)));
  tasks.emplace_after(tasks.before_begin(), part.into_send_task_ptr());
  return tasks;
}
HTTP_NAMESPACE_END
