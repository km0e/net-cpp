#pragma once
#ifndef XSL_SYS_DEF
#  define XSL_SYS_DEF
#  define XSL_SYS_NB namespace xsl::_sys {
#  define XSL_SYS_NE }
#  include <concepts>
XSL_SYS_NB
template <class T>
concept RawDeviceLike = requires(T t) {
  { t.raw() };
};

template <class T>
concept AsyncRawDeviceLike = RawDeviceLike<T> && requires(T t) {
  { t.sem() } -> std::same_as<typename T::sem_type &>;
};
XSL_SYS_NE
#endif
