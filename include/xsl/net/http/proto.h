#pragma once
#ifndef _XSL_NET_HTTP_PROTO_H_
#  define _XSL_NET_HTTP_PROTO_H_
#  include "xsl/net/http/def.h"

#  include <array>
#  include <regex>
#  include <string_view>

HTTP_NAMESPACE_BEGIN

enum class HttpVersion : uint8_t {
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

std::string_view to_string_view(const HttpVersion& method);

enum class HttpMethod : uint8_t {
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

std::string_view to_string_view(const HttpMethod& method);

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
  std::string_view to_string(Type type);
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
  };
  std::string to_string(const MediaType& media_type);
  std::string& operator+=(std::string& lhs, const MediaType& rhs);
}  // namespace content_type

class ContentType {
public:
  ContentType();
  ContentType(content_type::MediaType media_type, Charset charset);
  ~ContentType();
  content_type::MediaType media_type;
  Charset charset;
};

std::string to_string(const ContentType& content_type);
std::string& operator+=(std::string& lhs, const ContentType& rhs);

HTTP_NAMESPACE_END

#endif
