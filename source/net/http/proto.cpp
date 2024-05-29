#include "xsl/net/http/def.h"
#include "xsl/net/http/proto.h"
#include "xsl/wheel.h"
#include "xsl/wheel/def.h"
HTTP_NAMESPACE_BEGIN

string_view to_string_view(const HttpVersion& version) {
  if (version == HttpVersion::UNKNOWN) return "Unknown";
  return HTTP_VERSION_STRINGS[static_cast<uint8_t>(version)];
}

string_view to_string_view(const HttpMethod& method) {
  if (method == HttpMethod::UNKNOWN) return "Unknown";
  return HTTP_METHOD_STRINGS[static_cast<uint8_t>(method)];
}

string_view to_string_view(const Charset& charset) {
  if (charset == Charset::UNKNOWN) return "Unknown";
  return CHARSET_STRINGS[static_cast<uint8_t>(charset)];
}

namespace content_type {
  string_view to_string(Type type) {
    if (type == Type::UNKNOWN) return "Unknown";
    return TYPE_STRINGS[static_cast<uint8_t>(type)];
  }
  string& operator+=(string& lhs, Type rhs) {
    return lhs.append(TYPE_STRINGS[static_cast<uint8_t>(rhs)]);
  }
  string_view to_string_view(const SubType& subtype) {
    if (subtype == SubType::UNKNOWN) return "Unknown";
    return SUB_TYPE_STRINGS[static_cast<uint8_t>(subtype)];
  }
  string& operator+=(string& lhs, SubType rhs) {
    return lhs.append(SUB_TYPE_STRINGS[static_cast<uint8_t>(rhs)]);
  }
  MediaType MediaType::from_extension(string_view extension) {
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
  string to_string(const MediaType& media_type) {
    return string{to_string(media_type.type)}.append("/").append(
        to_string_view(media_type.sub_type));
  }
  string& operator+=(string& lhs, MediaType rhs) {
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
string to_string(const ContentType& content_type) {
  string result{to_string(content_type.media_type)};
  if (content_type.charset != Charset::UNKNOWN) {
    result += "; charset=";
    result += to_string_view(content_type.charset);
  }
  return result;
}

HTTP_NAMESPACE_END

WHEEL_NAMESPACE_BEGIN
template <>
net::http::HttpVersion from_string<net::http::HttpVersion>(string_view version) {
  auto iter = ranges::find(net::http::HTTP_VERSION_STRINGS, version);
  if (iter == net::http::HTTP_VERSION_STRINGS.end()) return net::http::HttpVersion::UNKNOWN;
  return static_cast<net::http::HttpVersion>(iter - net::http::HTTP_VERSION_STRINGS.begin());
}
template <>
net::http::HttpMethod from_string<net::http::HttpMethod>(string_view method) {
  auto iter = ranges::find(net::http::HTTP_METHOD_STRINGS, method);
  if (iter == net::http::HTTP_METHOD_STRINGS.end()) return net::http::HttpMethod::UNKNOWN;
  return static_cast<net::http::HttpMethod>(iter - net::http::HTTP_METHOD_STRINGS.begin());
}
template <>
net::http::content_type::Type from_string<net::http::content_type::Type>(string_view type) {
  auto iter = ranges::find(net::http::content_type::TYPE_STRINGS, type);
  if (iter == net::http::content_type::TYPE_STRINGS.end())
    return net::http::content_type::Type::UNKNOWN;
  return static_cast<net::http::content_type::Type>(
      iter - net::http::content_type::TYPE_STRINGS.begin());
}
template <>
net::http::content_type::SubType from_string<net::http::content_type::SubType>(
    string_view subtype) {
  auto iter = ranges::find(net::http::content_type::SUB_TYPE_STRINGS, subtype);
  if (iter == net::http::content_type::SUB_TYPE_STRINGS.end())
    return net::http::content_type::SubType::UNKNOWN;
  return static_cast<net::http::content_type::SubType>(
      iter - net::http::content_type::SUB_TYPE_STRINGS.begin());
}
template <>
net::http::content_type::MediaType from_string(string_view type) {
  auto pos = type.find('/');
  if (pos == string_view::npos) return net::http::content_type::MediaType{};
  auto type_str = type.substr(0, pos);
  auto sub_type_str = type.substr(pos + 1);
  auto type_enum = from_string<net::http::content_type::Type>(type_str);
  auto sub_type_enum = from_string<net::http::content_type::SubType>(sub_type_str);
  return net::http::content_type::MediaType{type_enum, sub_type_enum};
}

WHEEL_NAMESPACE_END
