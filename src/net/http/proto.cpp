#include "xsl/net/http/def.h"
#include "xsl/net/http/proto.h"
#include "xsl/wheel/def.h"
HTTP_NAMESPACE_BEGIN

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

HTTP_NAMESPACE_END
