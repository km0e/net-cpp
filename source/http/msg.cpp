#include <xsl/http/http.h>
#include <xsl/http/msg.h>
HTTP_NAMESPACE_BEGIN
wheel::string method_cast(HttpMethod method) {
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
HttpMethod method_cast(wheel::string_view method) {
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
RequestView::RequestView() {}
RequestView::~RequestView() {}
Request::Request(wheel::string raw, RequestView view) : raw(raw), view(view) {
  method = method_cast(view.method);
}
Request::~Request() {}
ResponseError::ResponseError(int code, wheel::string_view message) : code(code), message(message) {}
ResponseError::~ResponseError() {}
Response::Response() {}
Response::Response(wheel::string version, int status_code, wheel::string status_message)
    : version(version), status_code(status_code), status_message(status_message) {}
Response::~Response() {}
wheel::string Response::to_string() const {
  wheel::string res;
  res.reserve(1024);
  res += version;
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
  res += body;
  return res;
}

HTTP_NAMESPACE_END
