#pragma once
#include <chrono>
#ifndef XSL_NET_HTTP_PROTO
#  define XSL_NET_HTTP_PROTO
#  include "xsl/net/http/def.h"

#  include <array>
#  include <regex>
#  include <string_view>

XSL_HTTP_NB

enum class Version : uint8_t {
  EXT,
  HTTP_1_0,
  HTTP_1_1,
  HTTP_2_0,
  UNKNOWN = 0xff,
};

const int HTTP_VERSION_COUNT = 4;
const std::array<std::string_view, HTTP_VERSION_COUNT> HTTP_VERSION_STRINGS = {
    "EXT",
    "HTTP/1.0",
    "HTTP/1.1",
    "HTTP/2.0",
};

const std::regex HTTP_VERSION_REGEX = std::regex(R"(HTTP/(\d)\.(\d))");

std::string_view to_string_view(const Version& method);

enum class Method : uint8_t {
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
};

const int HTTP_METHOD_COUNT = 9;
const std::array<std::string_view, HTTP_METHOD_COUNT> HTTP_METHOD_STRINGS = {
    "EXT", "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "TRACE", "CONNECT",
};

std::string_view to_string_view(const Method& method);

enum class Charset : uint8_t {
  UTF_8,
  GBK,
  GB2312,
  ISO_8859_1,
  UNKNOWN = 0xff,
};

const int CHARSET_COUNT = 4;

const std::array<std::string_view, CHARSET_COUNT> CHARSET_STRINGS = {
    "UTF-8",
    "GBK",
    "GB2312",
    "ISO-8859-1",
};

std::string_view to_string_view(const Charset& charset);

namespace content_type {
  enum class Type : uint8_t {
    TEXT,
    APPLICATION,
    MULTIPART,
    UNKNOWN = 0xff,
  };
  const int TYPE_COUNT = 3;
  const std::array<std::string_view, TYPE_COUNT> TYPE_STRINGS = {
      "text",
      "application",
      "multipart",
  };
  std::string_view to_string_view(Type type);
  std::string& operator+=(std::string& lhs, Type rhs);
  enum class SubType : uint8_t {
    PLAIN,
    HTML,
    XML,
    CSS,
    JAVASCRIPT,
    JSON,
    XHTML,
    OCTET_STREAM,
    FORM_URLENCODED,
    FORM_DATA,
    UNKNOWN = 0xff,
  };
  const int SUB_TYPE_COUNT = 10;
  const std::array<std::string_view, SUB_TYPE_COUNT> SUB_TYPE_STRINGS = {
      "plain",     "html",         "xml",
      "css",       "javascript",   "json",
      "xhtml",     "octet-stream", "x-www-form-urlencoded",
      "form-data",
  };
  std::string_view to_string_view(const SubType& type);
  std::string& operator+=(std::string& lhs, SubType rhs);
  class MediaType {
  public:
    static MediaType from_extension(std::string_view extension);
    MediaType();
    MediaType(Type type, SubType sub_type);
    ~MediaType();
    Type type;
    SubType sub_type;
    std::string to_string();
  };
  std::string& operator+=(std::string& lhs, const MediaType& rhs);
}  // namespace content_type

class ContentType {
public:
  ContentType();
  ContentType(content_type::MediaType media_type, Charset charset);
  ~ContentType();
  content_type::MediaType media_type;
  Charset charset;
  std::string to_string();
};

std::string& operator+=(std::string& lhs, const ContentType& rhs);

enum class Status : uint16_t {
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
};

const int HTTP_STATUS_COUNT = 45;
const std::array<std::string_view, HTTP_STATUS_COUNT> HTTP_STATUS_STRINGS = {
    "100", "101", "200", "201", "202", "203", "204", "205", "206",     "300", "301", "302",
    "303", "304", "305", "307", "308", "400", "401", "402", "403",     "404", "405", "406",
    "407", "408", "409", "410", "411", "412", "413", "414", "415",     "416", "417", "421",
    "422", "426", "500", "501", "502", "503", "504", "505", "UNKNOWN",
};

std::string_view to_string_view(Status status);

const std::array<std::string_view, HTTP_STATUS_COUNT> HTTP_REASON_PHRASES = {
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

std::string_view to_reason_phrase(Status status);

template <class Clock, class Duration>
[[nodiscard("The return http date string should be used")]]
std::string to_date_string(const std::chrono::time_point<Clock, Duration>& time) {
  return std::format("{:%a, %d %b %Y %T %Z}",
                     std::chrono::time_point_cast<std::chrono::seconds>(time));
}

XSL_HTTP_NE

#  include "xsl/convert.h"
XSL_NB
template <>
_net::http::Version from_string_view(std::string_view type);

template <>
_net::http::Method from_string_view(std::string_view type);

template <>
_net::http::content_type::Type from_string_view(std::string_view type);

template <>
_net::http::content_type::SubType from_string_view(std::string_view type);

template <>
_net::http::content_type::MediaType from_string_view(std::string_view type);
XSL_NE
#endif
