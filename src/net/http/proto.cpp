#include "xsl/net/http/def.h"
#include "xsl/net/http/proto.h"
XSL_HTTP_NB

std::string_view to_string_view(const Version& version) {
  if (version == Version::UNKNOWN) return "Unknown";
  return HTTP_VERSION_STRINGS[static_cast<uint8_t>(version)];
}

std::string_view to_string_view(const Method& method) {
  if (method == Method::UNKNOWN) return "Unknown";
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

static uint16_t to_index(Status status) {
  switch (status) {
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

std::string_view to_string_view(Status status) { return HTTP_STATUS_STRINGS[to_index(status)]; }

std::string_view to_reason_phrase(Status status) {
  return HTTP_REASON_PHRASES[to_index(status)];
}

XSL_HTTP_NE

#include "xsl/def.h"

XSL_NB
template <>
_net::http::Version from_string_view<_net::http::Version>(std::string_view version) {
  auto iter = std::ranges::find(_net::http::HTTP_VERSION_STRINGS, version);
  if (iter == _net::http::HTTP_VERSION_STRINGS.end()) return _net::http::Version::UNKNOWN;
  return static_cast<_net::http::Version>(iter - _net::http::HTTP_VERSION_STRINGS.begin());
}
template <>
_net::http::Method from_string_view<_net::http::Method>(std::string_view method) {
  auto iter = std::ranges::find(_net::http::HTTP_METHOD_STRINGS, method);
  if (iter == _net::http::HTTP_METHOD_STRINGS.end()) return _net::http::Method::UNKNOWN;
  return static_cast<_net::http::Method>(iter - _net::http::HTTP_METHOD_STRINGS.begin());
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

XSL_HTTP_NE
