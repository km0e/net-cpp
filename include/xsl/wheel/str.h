#pragma once
#ifndef _XSL_WHEEL_STR_H_
#  define _XSL_WHEEL_STR_H_
#  include "xsl/wheel/def.h"
#  include "xsl/wheel/giant.h"

#  include <string>

WHEEL_NAMESPACE_BEGIN
using std::to_string;
template <typename T>
concept ToString = requires(T t) {
  { to_string(t) } -> giant::convertible_to<giant::string_view>;
} || requires(T t) {
  { t.to_string() } -> giant::convertible_to<giant::string_view>;
};

template <typename T>
concept Stringable = requires(T t) {
  { t.to_string() } -> giant::convertible_to<giant::string_view>;
};

template <Stringable T>
giant::string to_string(T t) {
  return t.to_string();
}
template <typename T>
T from_string(giant::string_view str);

WHEEL_NAMESPACE_END
#endif
