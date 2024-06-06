#pragma once
#ifndef _XSL_WHEEL_UTILS_H_
#  define _XSL_WHEEL_UTILS_H_
#  include "xsl/wheel/def.h"
#  include "xsl/wheel/str.h"

#  include <functional>
#  include <string>
WHEEL_NAMESPACE_BEGIN
namespace detail {

  class string_hasher : public std::hash<std::string>, public std::hash<std::string_view> {
  public:
    using is_transparent = void;
    using std::hash<std::string>::operator();
    using std::hash<std::string_view>::operator();
    auto operator()(const char* str) const { return this->operator()(std::string_view(str)); }
    auto operator()(const FixedString& str) const { return this->operator()(str.to_string_view()); }
  };
}  // namespace detail
template <typename T>
using sumap = std::unordered_map<std::string, T, detail::string_hasher, std::equal_to<>>;
WHEEL_NAMESPACE_END
#endif
