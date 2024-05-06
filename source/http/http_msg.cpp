#include <xsl/http/config.h>
#include <xsl/http/http_msg.h>
HTTP_NAMESPACE_BEGIN
RequestError::RequestError(RequestErrorKind kind) : kind(kind) {}
RequestError::RequestError(RequestErrorKind kind, wheel::string message)
    : kind(kind), message(message) {}
RequestError::~RequestError() {}
HttpRequest::HttpRequest() {}
HttpRequest::~HttpRequest() {}
ResponseError::ResponseError(int code, wheel::string_view message) : code(code), message(message) {}
ResponseError::~ResponseError() {}
HttpResponse::HttpResponse() {}
HttpResponse::~HttpResponse() {}
wheel::string HttpResponse::to_string() const {
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
