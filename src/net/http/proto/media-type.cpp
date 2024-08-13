#include "xsl/net/http/proto/def.h"
#include "xsl/net/http/proto/media-type.h"
XSL_NET_HTTP_PROTO_NB
std::string_view to_string_view(const MediaMainType &type) {
  return MEDIA_MAIN_TYPE_STRINGS[static_cast<std::size_t>(type)];
}

bool operator==(MediaMainType lhs, std::string_view rhs) { return to_string_view(lhs) == rhs; }

std::string_view to_string_view(const MediaSubType &subtype) {
  return MEDIA_SUB_TYPE_STRINGS[static_cast<std::size_t>(subtype)];
}

bool operator==(MediaSubType lhs, std::string_view rhs) { return to_string_view(lhs) == rhs; }

MediaType MediaType::from_extension(std::string_view extension) {
  if (extension == ".html" || extension == ".htm") {
    return {MediaMainType::TEXT, MediaSubType::HTML};
  } else if (extension == ".css") {
    return {MediaMainType::TEXT, MediaSubType::CSS};
  } else if (extension == ".js") {
    return {MediaMainType::APPLICATION, MediaSubType::JAVASCRIPT};
  } else if (extension == ".json") {
    return {MediaMainType::APPLICATION, MediaSubType::JSON};
  } else if (extension == ".xml") {
    return {MediaMainType::APPLICATION, MediaSubType::XML};
  } else {
    // TODO: add more content type
    return {MediaMainType::APPLICATION, MediaSubType::OCTET_STREAM};
  }
}
MediaType::MediaType() : main_type(MediaMainType::ANY), sub_type(MediaSubType::ANY), parameters() {}
MediaType::MediaType(MediaMainType main_type, MediaSubType sub_type)
    : main_type(main_type), sub_type(sub_type), parameters() {}

std::string MediaType::to_string() const {
  auto type = std::string{to_string_view(this->main_type)}.append("/").append(
      to_string_view(this->sub_type));
  for (const auto &[key, value] : this->parameters) {
    type.append("; ").append(key).append("=").append(value);
  }
  return type;
}

std::string_view MediaTypeView::main_type() const { return {start, slash}; }
std::string_view MediaTypeView::sub_type() const { return {slash + 1, end}; }

XSL_NET_HTTP_PROTO_NE
