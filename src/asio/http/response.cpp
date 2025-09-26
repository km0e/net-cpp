/**
 * @file response.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2025-07-11
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <xsl/asio/http/response.h>
#include <xsl/net.h>

XSL_ASIO_NB

ResponsePart::ResponsePart()
    : ResponsePart(Version::HTTP_1_1, Status::OK, Status{Status::OK}.to_reason_phrase()) {}

ResponsePart::ResponsePart(Version version, Status status_code, std::string_view&& status_message)
    : status_code(status_code),
      status_message(std::move(status_message)),
      version(version),
      headers() {}
ResponsePart::ResponsePart(Version version, Status status_code)
    : ResponsePart(version, status_code, status_code.to_reason_phrase()) {}
ResponsePart::ResponsePart(Version version, uint16_t status_code)
    : ResponsePart(version, Status{status_code}) {}
ResponsePart::ResponsePart(Status status_code) : ResponsePart(Version::HTTP_1_1, status_code) {}
ResponsePart::ResponsePart(uint16_t status_code) : ResponsePart(Version::HTTP_1_1, status_code) {}

ResponsePart::~ResponsePart() {}
std::string ResponsePart::to_string() {
  std::string res;
  res.reserve(1024);
  res += version.to_string_view();
  res += " ";
  res += status_code.to_string_view();
  res += " ";
  res += status_message;
  res += CRLF;
  for (const auto& [key, value] : headers) {
    res += key;
    res += ": ";
    res += value;
    res += CRLF;
  }
  if (!headers.contains("Server")) {
    res += "Server: ";
    res += API_VERSION;
    res += CRLF;
  }
  res += CRLF;
  return res;
}
XSL_ASIO_NE
