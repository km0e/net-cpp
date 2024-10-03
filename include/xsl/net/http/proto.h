/**
 * @file proto.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-25
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#include <cstddef>
#ifndef XSL_NET_HTTP_PROTO_
#  define XSL_NET_HTTP_PROTO_
#  include "xsl/net/http/def.h"

#  include <chrono>
#  include <optional>
#  include <string_view>

XSL_HTTP_NB

const std::string_view HTTP_VERSION_STR[] = {
    "EXT",
    "HTTP/1.0",
    "HTTP/1.1",
    "HTTP/2.0",
};

struct Version {
  enum : uint8_t {
    EXT,
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2_0,
    UNKNOWN = 0xff,
  } _version;
  static constexpr Version from_string_view(std::string_view str) {
    for (std::size_t i = 0; i < 4; i++) {
      if (str == HTTP_VERSION_STR[i]) return Version(static_cast<decltype(_version)>(i));
    }
    return Version(UNKNOWN);
  }

  Version() = default;
  Version(decltype(_version) version) : _version(version) {}

  constexpr std::string_view to_string_view() const {
    if (_version == UNKNOWN) return "Unknown";
    return HTTP_VERSION_STR[_version];
  }
};

constexpr bool operator==(const Version& lhs, const decltype(Version::_version)& rhs) {
  return lhs._version == rhs;
}

const std::size_t HTTP_METHOD_COUNT = 9;

const std::string_view HTTP_METHOD_STR[] = {
    "EXT", "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "TRACE", "CONNECT",
};

struct Method {
  enum : uint8_t {
    EXT,
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    TRACE,
    CONNECT,
    UNKNOWN = 0xff,
  } _method;
  static constexpr Method from_string_view(std::string_view str) {
    for (std::size_t i = 0; i < HTTP_METHOD_COUNT; i++) {
      if (str == HTTP_METHOD_STR[i]) return Method(i);
    }
    return Method(UNKNOWN);
  }

  constexpr Method(decltype(_method) method) : _method(method) {}

  constexpr Method(std::uint8_t method) : _method(static_cast<decltype(_method)>(method)) {}

  constexpr std::string_view to_string_view() const {
    if (_method == UNKNOWN) return "Unknown";
    return HTTP_METHOD_STR[_method];
  }
};

constexpr bool operator==(const Method& lhs, const decltype(Method::_method)& rhs) {
  return lhs._method == rhs;
}

const std::string_view CHARSET_STR[] = {
    "UTF-8",
    "GBK",
    "GB2312",
    "ISO-8859-1",
};

struct Charset {
  enum : uint8_t {
    UTF_8,
    GBK,
    GB2312,
    ISO_8859_1,
    UNKNOWN = 0xff,
  } _charset;

  constexpr std::string_view to_string_view() const {
    if (_charset == UNKNOWN) return "Unknown";
    return CHARSET_STR[_charset];
  }
};
const std::string_view HTTP_STATUS_STR[] = {
    "100", "101", "200", "201", "202", "203", "204", "205", "206",     "300", "301", "302",
    "303", "304", "305", "307", "308", "400", "401", "402", "403",     "404", "405", "406",
    "407", "408", "409", "410", "411", "412", "413", "414", "415",     "416", "417", "421",
    "422", "426", "500", "501", "502", "503", "504", "505", "UNKNOWN",
};

const std::string_view HTTP_REASON_PHRASES[] = {
    "Continue",
    "Switching Protocols",
    "OK",
    "Created",
    "Accepted",
    "Non-Authoritative Information",
    "No Content",
    "Reset Content",
    "Partial Content",
    "Multiple Choices",
    "Moved Permanently",
    "Found",
    "See Other",
    "Not Modified",
    "Use Proxy",
    "Temporary Redirect",
    "Permanent Redirect",
    "Bad Request",
    "Unauthorized",
    "Payment Required",
    "Forbidden",
    "Not Found",
    "Method Not Allowed",
    "Not Acceptable",
    "Proxy Authentication Required",
    "Request Timeout",
    "Conflict",
    "Gone",
    "Length Required",
    "Precondition Failed",
    "Payload Too Large",
    "URI Too Long",
    "Unsupported Media Type",
    "Range Not Satisfiable",
    "Expectation Failed",
    "Misdirected Request",
    "Unprocessable Content",
    "Upgrade Required",
    "Internal Server Error",
    "Not Implemented",
    "Bad Gateway",
    "Service Unavailable",
    "Gateway Timeout",
    "HTTP Version Not Supported",
};
struct Status;

struct Status {
  enum : uint16_t {
    CONTINUE = 100,
    SWITCHING_PROTOCOLS = 101,
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NON_AUTHORITATIVE_INFORMATION = 203,
    NO_CONTENT = 204,
    RESET_CONTENT = 205,
    PARTIAL_CONTENT = 206,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    USE_PROXY = 305,
    TEMPORARY_REDIRECT = 307,
    PERMANENT_REDIRECT = 308,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    PAYMENT_REQUIRED = 402,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    NOT_ACCEPTABLE = 406,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    REQUEST_TIMEOUT = 408,
    CONFLICT = 409,
    GONE = 410,
    LENGTH_REQUIRED = 411,
    PRECONDITION_FAILED = 412,
    PAYLOAD_TOO_LARGE = 413,
    URI_TOO_LONG = 414,
    UNSUPPORTED_MEDIA_TYPE = 415,
    RANGE_NOT_SATISFIABLE = 416,
    EXPECTATION_FAILED = 417,
    MISDIRECTED_REQUEST = 421,
    UNPROCESSABLE_CONTENT = 422,
    UPGRADE_REQUIRED = 426,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505,
    UNKNOWN = 0xffff,
  } _status;

  constexpr Status() : _status(UNKNOWN) {}
  constexpr Status(decltype(_status) status) : _status(status) {}
  constexpr Status(uint16_t status) : _status(static_cast<decltype(_status)>(status)) {}

  constexpr std::string_view to_string_view() { return HTTP_STATUS_STR[this->to_index()]; }

  constexpr std::string_view to_reason_phrase() { return HTTP_REASON_PHRASES[this->to_index()]; }

private:
  constexpr uint16_t to_index() {
    switch (_status) {
      case Status::CONTINUE:
        return 0;
      case Status::SWITCHING_PROTOCOLS:
        return 1;
      case Status::OK:
        return 2;
      case Status::CREATED:
        return 3;
      case Status::ACCEPTED:
        return 4;
      case Status::NON_AUTHORITATIVE_INFORMATION:
        return 5;
      case Status::NO_CONTENT:
        return 6;
      case Status::RESET_CONTENT:
        return 7;
      case Status::PARTIAL_CONTENT:
        return 8;
      case Status::MULTIPLE_CHOICES:
        return 9;
      case Status::MOVED_PERMANENTLY:
        return 10;
      case Status::FOUND:
        return 11;
      case Status::SEE_OTHER:
        return 12;
      case Status::NOT_MODIFIED:
        return 13;
      case Status::USE_PROXY:
        return 14;
      case Status::TEMPORARY_REDIRECT:
        return 15;
      case Status::PERMANENT_REDIRECT:
        return 16;
      case Status::BAD_REQUEST:
        return 17;
      case Status::UNAUTHORIZED:
        return 18;
      case Status::PAYMENT_REQUIRED:
        return 19;
      case Status::FORBIDDEN:
        return 20;
      case Status::NOT_FOUND:
        return 21;
      case Status::METHOD_NOT_ALLOWED:
        return 22;
      case Status::NOT_ACCEPTABLE:
        return 23;
      case Status::PROXY_AUTHENTICATION_REQUIRED:
        return 24;
      case Status::REQUEST_TIMEOUT:
        return 25;
      case Status::CONFLICT:
        return 26;
      case Status::GONE:
        return 27;
      case Status::LENGTH_REQUIRED:
        return 28;
      case Status::PRECONDITION_FAILED:
        return 29;
      case Status::PAYLOAD_TOO_LARGE:
        return 30;
      case Status::URI_TOO_LONG:
        return 31;
      case Status::UNSUPPORTED_MEDIA_TYPE:
        return 32;
      case Status::RANGE_NOT_SATISFIABLE:
        return 33;
      case Status::EXPECTATION_FAILED:
        return 34;
      case Status::MISDIRECTED_REQUEST:
        return 35;
      case Status::UNPROCESSABLE_CONTENT:
        return 36;
      case Status::UPGRADE_REQUIRED:
        return 37;
      case Status::INTERNAL_SERVER_ERROR:
        return 38;
      case Status::NOT_IMPLEMENTED:
        return 39;
      case Status::BAD_GATEWAY:
        return 40;
      case Status::SERVICE_UNAVAILABLE:
        return 41;
      case Status::GATEWAY_TIMEOUT:
        return 42;
      case Status::HTTP_VERSION_NOT_SUPPORTED:
        return 43;
      default:
        return 44;
    }
  }
};

constexpr bool operator==(const Status& lhs, const decltype(Status::_status)& rhs) {
  return lhs._status == rhs;
}

constexpr bool operator==(const Status& lhs, const Status& rhs) {
  return lhs._status == rhs._status;
}

template <class Clock, class Duration>
[[nodiscard("The return http date string should be used")]]
constexpr std::string to_date_string(const std::chrono::time_point<Clock, Duration>& time) {
  return std::format("{:%a, %d %b %Y %T %Z}",
                     std::chrono::time_point_cast<std::chrono::seconds>(time));
}

template <class Clock, class Duration = typename Clock::duration>
[[nodiscard("The return time point should be used")]]
constexpr std::optional<std::chrono::time_point<Clock, Duration>> from_date_string(
    std::string_view date) {
  std::chrono::time_point<Clock, Duration> ft;
  std::istringstream ss{std::string(date)};
  std::chrono::from_stream(ss, "%a, %d %b %Y %T %Z", ft);
  if (ss.fail()) {
    return std::nullopt;
  }
  return ft;
}

XSL_HTTP_NE
namespace std {
  using namespace xsl::_net::http;
  template <>
  struct hash<Status> : public std::hash<decltype(Status::_status)> {
    constexpr size_t operator()(const Status& status) const noexcept {
      return std::hash<decltype(Status::_status)>::operator()(status._status);
    }
  };
}  // namespace std
#endif
