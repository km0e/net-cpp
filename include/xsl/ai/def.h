#pragma once
#ifndef XSL_AI_DEF
#  define XSL_AI_DEF
#  define XSL_AI_NB namespace xsl::_ai {
#  define XSL_AI_NE }
#  include <type_traits>
XSL_AI_NB
template <class T>
struct DeviceTraits;

template <>
struct DeviceTraits<void> {
  using value_type = void;
};

template <class T>
struct DeviceTraits<std::type_identity<T>> {
  using value_type = T;
};

using DefaultDeviceTraits = DeviceTraits<void>;
XSL_AI_NE
#endif
