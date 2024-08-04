#include "xsl/net/http/def.h"
#include "xsl/net/http/proto.h"
HTTP_NB

std::string_view to_string_view(const HttpVersion& version) {
  if (version == HttpVersion::UNKNOWN) return "Unknown";
  return HTTP_VERSION_STRINGS[static_cast<uint8_t>(version)];
}

std::string_view to_string_view(const HttpMethod& method) {
  if (method == HttpMethod::UNKNOWN) return "Unknown";
  return HTTP_METHOD_STRINGS[static_cast<uint8_t>(method)];
}

std::string_view to_string_view(const Charset& charset) {
  if (charset == Charset::UNKNOWN) return "Unknown";
  return CHARSET_STRINGS[static_cast<uint8_t>(charset)];
}

namespace content_type {
  std::string_view to_string(Type type) {
    if (type == Type::UNKNOWN) return "Unknown";
    return TYPE_STRINGS[static_cast<uint8_t>(type)];
  }
  std::string& operator+=(std::string& lhs, Type rhs) {
    return lhs.append(TYPE_STRINGS[static_cast<uint8_t>(rhs)]);
  }
  std::string_view to_string_view(const SubType& subtype) {
    if (subtype == SubType::UNKNOWN) return "Unknown";
    return SUB_TYPE_STRINGS[static_cast<uint8_t>(subtype)];
  }
  std::string& operator+=(std::string& lhs, SubType rhs) {
    return lhs.append(SUB_TYPE_STRINGS[static_cast<uint8_t>(rhs)]);
  }
  MediaType MediaType::from_extension(std::string_view extension) {
    if (extension == "html" || extension == "htm") {
      return {Type::TEXT, SubType::HTML};
    } else if (extension == "css") {
      return {Type::TEXT, SubType::CSS};
    } else if (extension == "js") {
      return {Type::APPLICATION, SubType::JAVASCRIPT};
    } else if (extension == "json") {
      return {Type::APPLICATION, SubType::JSON};
    } else if (extension == "xml") {
      return {Type::APPLICATION, SubType::XML};
    } else {
      // TODO: add more content type
      return {Type::APPLICATION, SubType::OCTET_STREAM};
    }
  }

  MediaType::MediaType() : type(Type::UNKNOWN), sub_type(SubType::UNKNOWN) {}
  MediaType::MediaType(Type type, SubType sub_type) : type(type), sub_type(sub_type) {}
  MediaType::~MediaType() {}
  std::string to_string(const MediaType& media_type) {
    return std::string{to_string(media_type.type)}.append("/").append(
        to_string_view(media_type.sub_type));
  }
  std::string& operator+=(std::string& lhs, MediaType rhs) {
    lhs += to_string(rhs.type);
    lhs += '/';
    lhs += to_string_view(rhs.sub_type);
    return lhs;
  }
}  // namespace content_type
ContentType::ContentType() : media_type(content_type::MediaType{}), charset(Charset::UNKNOWN) {}
ContentType::ContentType(content_type::MediaType media_type, Charset charset)
    : media_type(media_type), charset(charset) {}
ContentType::~ContentType() {}
std::string to_string(const ContentType& content_type) {
  std::string result{to_string(content_type.media_type)};
  if (content_type.charset != Charset::UNKNOWN) {
    result += "; charset=";
    result += to_string_view(content_type.charset);
  }
  return result;
}

std::string_view to_string_view(HttpStatus status) {
  switch (status) {
    case HttpStatus::CONTINUE:
      return "Continue";
    case HttpStatus::SWITCHING_PROTOCOLS:
      return "Switching Protocols";
    case HttpStatus::OK:
      return "OK";
    case HttpStatus::CREATED:
      return "Created";
    case HttpStatus::ACCEPTED:
      return "Accepted";
    case HttpStatus::NON_AUTHORITATIVE_INFORMATION:
      return "Non-Authoritative Information";
    case HttpStatus::NO_CONTENT:
      return "No Content";
    case HttpStatus::RESET_CONTENT:
      return "Reset Content";
    case HttpStatus::PARTIAL_CONTENT:
      return "Partial Content";
    case HttpStatus::MULTIPLE_CHOICES:
      return "Multiple Choices";
    case HttpStatus::MOVED_PERMANENTLY:
      return "Moved Permanently";
    case HttpStatus::FOUND:
      return "Found";
    case HttpStatus::SEE_OTHER:
      return "See Other";
    case HttpStatus::NOT_MODIFIED:
      return "Not Modified";
    case HttpStatus::USE_PROXY:
      return "Use Proxy";
    case HttpStatus::TEMPORARY_REDIRECT:
      return "Temporary Redirect";
    case HttpStatus::PERMANENT_REDIRECT:
      return "Permanent Redirect";
    case HttpStatus::BAD_REQUEST:
      return "Bad Request";
    case HttpStatus::UNAUTHORIZED:
      return "Unauthorized";
    case HttpStatus::PAYMENT_REQUIRED:
      return "Payment Required";
    case HttpStatus::FORBIDDEN:
      return "Forbidden";
    case HttpStatus::NOT_FOUND:
      return "Not Found";
    case HttpStatus::METHOD_NOT_ALLOWED:
      return "Method Not Allowed";
    case HttpStatus::NOT_ACCEPTABLE:
      return "Not Acceptable";
    case HttpStatus::PROXY_AUTHENTICATION_REQUIRED:
      return "Proxy Authentication Required";
    case HttpStatus::REQUEST_TIMEOUT:
      return "Request Timeout";
    case HttpStatus::CONFLICT:
      return "Conflict";
    case HttpStatus::GONE:
      return "Gone";
    case HttpStatus::LENGTH_REQUIRED:
      return "Length Required";
    case HttpStatus::PRECONDITION_FAILED:
      return "Precondition Failed";
    case HttpStatus::PAYLOAD_TOO_LARGE:
      return "Payload Too Large";
    case HttpStatus::URI_TOO_LONG:
      return "URI Too Long";
    case HttpStatus::UNSUPPORTED_MEDIA_TYPE:
      return "Unsupported Media Type";
    case HttpStatus::RANGE_NOT_SATISFIABLE:
      return "Range Not Satisfiable";
    case HttpStatus::EXPECTATION_FAILED:
      return "Expectation Failed";
    case HttpStatus::MISDIRECTED_REQUEST:
      return "Misdirected Request";
    case HttpStatus::UNPROCESSABLE_CONTENT:
      return "Unprocessable Content";
    case HttpStatus::UPGRADE_REQUIRED:
      return "Upgrade Required";
    case HttpStatus::INTERNAL_SERVER_ERROR:
      return "Internal Server Error";
    case HttpStatus::NOT_IMPLEMENTED:
      return "Not Implemented";
    case HttpStatus::BAD_GATEWAY:
      return "Bad Gateway";
    case HttpStatus::SERVICE_UNAVAILABLE:
      return "Service Unavailable";
    case HttpStatus::GATEWAY_TIMEOUT:
      return "Gateway Timeout";
    case HttpStatus::HTTP_VERSION_NOT_SUPPORTED:
      return "HTTP Version Not Supported";
    case HttpStatus::UNKNOWN:
      return "Unknown";
    default:
      return "Unknown";
  }
}

HTTP_NE

#include "xsl/def.h"

XSL_NB
template <>
_net::http::HttpVersion from_string_view<_net::http::HttpVersion>(std::string_view version) {
  auto iter = std::ranges::find(_net::http::HTTP_VERSION_STRINGS, version);
  if (iter == _net::http::HTTP_VERSION_STRINGS.end()) return _net::http::HttpVersion::UNKNOWN;
  return static_cast<_net::http::HttpVersion>(iter - _net::http::HTTP_VERSION_STRINGS.begin());
}
template <>
_net::http::HttpMethod from_string_view<_net::http::HttpMethod>(std::string_view method) {
  auto iter = std::ranges::find(_net::http::HTTP_METHOD_STRINGS, method);
  if (iter == _net::http::HTTP_METHOD_STRINGS.end()) return _net::http::HttpMethod::UNKNOWN;
  return static_cast<_net::http::HttpMethod>(iter - _net::http::HTTP_METHOD_STRINGS.begin());
}

template <>
_net::http::content_type::Type from_string_view<_net::http::content_type::Type>(
    std::string_view type) {
  auto iter = std::ranges::find(_net::http::content_type::TYPE_STRINGS, type);
  if (iter == _net::http::content_type::TYPE_STRINGS.end())
    return _net::http::content_type::Type::UNKNOWN;
  return static_cast<_net::http::content_type::Type>(
      iter - _net::http::content_type::TYPE_STRINGS.begin());
}
template <>
_net::http::content_type::SubType from_string_view<_net::http::content_type::SubType>(
    std::string_view subtype) {
  auto iter = std::ranges::find(_net::http::content_type::SUB_TYPE_STRINGS, subtype);
  if (iter == _net::http::content_type::SUB_TYPE_STRINGS.end())
    return _net::http::content_type::SubType::UNKNOWN;
  return static_cast<_net::http::content_type::SubType>(
      iter - _net::http::content_type::SUB_TYPE_STRINGS.begin());
}
template <>
_net::http::content_type::MediaType from_string_view(std::string_view type) {
  auto pos = type.find('/');
  if (pos == std::string_view::npos) return _net::http::content_type::MediaType{};
  auto type_str = type.substr(0, pos);
  auto sub_type_str = type.substr(pos + 1);
  auto type_enum = from_string_view<_net::http::content_type::Type>(type_str);
  auto sub_type_enum = from_string_view<_net::http::content_type::SubType>(sub_type_str);
  return _net::http::content_type::MediaType{type_enum, sub_type_enum};
}

XSL_NE
