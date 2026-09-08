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

#include <cstring>

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
  for (std::size_t i = 0; i < this->headers.size(); ++i) {
    auto entry = this->headers.entry(i);
    res += entry.key;
    res += ": ";
    res += entry.value;
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
std::string_view ResponsePart::render_head(char* buf, std::size_t cap) {
  std::size_t off = 0;
  auto append = [&](std::string_view sv) {
    if (off + sv.size() > cap) return false;
    std::memcpy(buf + off, sv.data(), sv.size());
    off += sv.size();
    return true;
  };
  if (!append(this->version.to_string_view()) || !append(" ")
      || !append(this->status_code.to_string_view()) || !append(" ")
      || !append(this->status_message) || !append(CRLF)) {
    return {};
  }
  for (std::size_t i = 0; i < this->headers.size(); ++i) {
    auto entry = this->headers.entry(i);
    if (!append(entry.key) || !append(": ") || !append(entry.value) || !append(CRLF)) return {};
  }
  if (!this->headers.contains("Server")) {
    if (!append("Server: ") || !append(API_VERSION) || !append(CRLF)) return {};
  }
  if (!append(CRLF)) return {};
  return {buf, off};
}
XSL_ASIO_NE
