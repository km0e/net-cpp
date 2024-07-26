#include "xsl/convert.h"
#include "xsl/net/http/msg.h"
#include "xsl/net/http/proto.h"
#include "xsl/sys/net/io.h"

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
HttpResponse::HttpResponse(ResponsePart&& part) : part(std::move(part)), body() {}

HttpResponse::~HttpResponse() {}
coro::Task<std::tuple<std::size_t, std::optional<std::errc>>> HttpResponse::sendto(
    sys::io::AsyncWriteDevice& awd) {
  auto str = this->part.to_string();
  auto [sz, err] = co_await sys::net::immediate_send(awd, std::as_bytes(std::span(str)));
  if (err) {
    co_return std::make_tuple(sz, err);
  };
  if (!body) {
    co_return std::make_tuple(sz, std::nullopt);
  }
  auto [bsz, berr] = co_await this->body(awd);
  if (berr) {
    co_return std::make_tuple(sz + bsz, berr);
  }
  co_return std::make_tuple(sz + bsz, std::nullopt);
}
HTTP_NE
