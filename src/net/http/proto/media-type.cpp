/**
 * @file media-type.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/net/http/proto/media-type.h"

#include <cstddef>
XSL_HTTP_NB
std::string_view to_string_view(const MediaMainType &type) {
  return MEDIA_MAIN_TYPE_STRINGS[static_cast<std::size_t>(type)];
}

bool operator==(MediaMainType lhs, std::string_view rhs) { return to_string_view(lhs) == rhs; }

std::string_view to_string_view(const MediaSubType &subtype) {
  return MEDIA_SUB_TYPE_STRINGS[static_cast<std::size_t>(subtype)];
}

bool operator==(MediaSubType lhs, std::string_view rhs) { return to_string_view(lhs) == rhs; }
MediaTypeBase MediaTypeBase::from_extension(std::string_view extension) {
  using namespace common_media_type;
  auto iter = extension_to_media_type.find(extension);
  if (iter != extension_to_media_type.end()) {
    auto &media_type = iter->second;
    auto slash = media_type.find('/');
    return {media_type.data(), media_type.data() + slash, media_type.data() + media_type.size()};
  }
  //<default is application/octet-stream>
  auto slash = application_octet_stream.find('/');
  return {application_octet_stream.data(), application_octet_stream.data() + slash,
          application_octet_stream.data()
              + application_octet_stream.size()};  // application/octet-stream
}
std::string_view MediaTypeBase::main_type() const { return {start, slash}; }
std::string_view MediaTypeBase::sub_type() const { return {slash + 1, end}; }
bool MediaTypeBase::type_includes(const MediaTypeBase &other) const {
  using namespace common_media_type;
  if (this->main_type() == any) {
    return true;
  }
  if (this->main_type() != other.main_type()) {
    return false;
  }
  if (this->sub_type() == any) {
    return true;
  }
  return this->sub_type() == other.sub_type();
}
std::string_view MediaTypeBase::to_string_view() const { return {start, end}; }

MediaType MediaType::from_extension(std::string_view extension) {
  auto base = MediaTypeBase::from_extension(extension);
  return {base, {}};
}
std::string MediaType::to_string() const {
  auto base_view = std::string{this->MediaTypeBase::to_string_view()};
  auto size = base_view.size();
  auto sum = std::ranges::fold_left(parameters, size, [](auto sum, auto &parameter) {
    return sum + 1 + parameter.name.size() + 1 + parameter.value.size();
  });
  std::string result;
  result.reserve(sum);
  result.append(base_view);
  for (auto &parameter : parameters) {
    result.push_back(';');
    result.append(parameter.name);
    result.push_back('=');
    result.append(parameter.value);
  }
  return result;
}
MediaTypeView MediaTypeView::from_extension(std::string_view extension) {
  auto base = MediaTypeBase::from_extension(extension);
  return {base, {}};
}
std::string_view MediaTypeView::to_string_view() const {
  return {start, parameters.empty() ? end : parameters.back().value.data()};
}

XSL_HTTP_NE
