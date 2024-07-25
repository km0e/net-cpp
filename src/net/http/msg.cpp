#include "xsl/convert.h"
#include "xsl/net/http/msg.h"
#include "xsl/net/http/proto.h"

#include <expected>

HTTP_NB

RequestView::RequestView() : method(), url(), query(), version(), headers(), length(0) {}

RequestView::~RequestView() {}

std::string RequestView::to_string() {
  std::string res;
  res.reserve(1024);
  res += method;
  res += " ";
  res += url;
  if (!query.empty()) {
    res += "?";
    for (const auto& [key, value] : query) {
      res += key;
      res += "=";
      res += value;
      res += "&";
    }
    res.pop_back();
  }
  res += " ";
  res += version;
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

void RequestView::clear() {
  method = std::string_view{};
  url = std::string_view{};
  query.clear();
  version = std::string_view{};
  headers.clear();
}

Request::Request(std::string&& raw, RequestView&& view, BodyStream&& body)
    : method(xsl::from_string_view<HttpMethod>(view.method)),
      view(std::move(view)),
      raw(std::move(raw)),
      body(std::move(body)) {}
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
coro::Task<std::expected<void, sys::io::SendError>> ResponsePart::operator()(
    sys::io::AsyncWriteDevice& awd) {
  auto str = this->to_string();
  auto res = co_await sys::io::immediate_write(awd, std::as_bytes(std::span(str)));
  if (!res) {
    co_return std::unexpected{res.error()};
  };
  co_return {};
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

HttpResponse<std::string>::HttpResponse() : part(), body() {}
HttpResponse<std::string>::HttpResponse(ResponsePart&& part, std::string&& body)
    : part(part), body(body) {}
HttpResponse<std::string>::~HttpResponse() {}
coro::Task<std::expected<void, sys::io::SendError>> HttpResponse<std::string>::operator()(
    sys::io::AsyncWriteDevice& awd) {
  co_await part(awd);
  auto res = co_await sys::io::immediate_write(awd, std::as_bytes(std::span(body)));
  if (!res) {
    co_return std::unexpected{res.error()};
  }
  co_return {};
}
HTTP_NE
