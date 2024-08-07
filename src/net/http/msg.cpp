#include "xsl/net/http/msg.h"
#include "xsl/net/http/proto.h"

#include <expected>

HTTP_NB

RequestView::RequestView()
    : method(), scheme(), authority(), path(), query(), version(), headers() {}

RequestView::~RequestView() {}

void RequestView::clear() {
  method = std::string_view{};
  query.clear();
  version = std::string_view{};
  headers.clear();
}

ResponseError::ResponseError(int code, std::string_view message) : code(code), message(message) {}
ResponseError::~ResponseError() {}

ResponsePart::ResponsePart()
    : ResponsePart(HttpVersion::HTTP_1_1, HttpStatus::OK, to_reason_phrase(HttpStatus::OK)) {}

ResponsePart::ResponsePart(HttpVersion version, HttpStatus status_code,
                           std::string_view&& status_message)
    : status_code(status_code),
      status_message(std::move(status_message)),
      version(version),
      headers() {}
ResponsePart::ResponsePart(HttpVersion version, HttpStatus status_code)
    : ResponsePart(version, status_code, to_reason_phrase(status_code)) {}
ResponsePart::ResponsePart(HttpVersion version, uint16_t status_code)
    : ResponsePart(version, static_cast<HttpStatus>(status_code)) {}

ResponsePart::~ResponsePart() {}
std::string ResponsePart::to_string() {
  std::string res;
  res.reserve(1024);
  res += http::to_string_view(version);
  res += " ";
  res += http::to_string_view(status_code);
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
HttpResponse::HttpResponse(ResponsePart&& part) : _part(std::move(part)), _body() {}

HttpResponse::~HttpResponse() {}

HTTP_NE
