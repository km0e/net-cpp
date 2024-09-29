/**
 * @file byte.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_BYTE
#  define XSL_BYTE
#  include "xsl/def.h"

#  include <cstdint>
#  include <span>
XSL_NB
using byte = std::uint8_t;

constexpr void bool_to_bytes(bool value, byte* bytes);

constexpr bool bool_from_bytes(const byte* bytes);

template <typename _Type, size_t _Extent>
  requires(!std::is_const_v<_Type>)
[[nodiscard]] constexpr std::span<byte, _Extent == std::dynamic_extent ? std::dynamic_extent
                                                                       : _Extent * sizeof(_Type)>
as_writable_bytes(std::span<_Type, _Extent> __sp) noexcept {
  auto data = reinterpret_cast<byte*>(__sp.data());
  auto size = __sp.size_bytes();
  constexpr auto extent
      = _Extent == std::dynamic_extent ? std::dynamic_extent : _Extent * sizeof(_Type);
  return std::span<byte, extent>{data, size};
}

template <class T, size_t _Extent = std::dynamic_extent>
[[nodiscard]]
constexpr std::span<byte, _Extent> as_writable_bytes(T* data, size_t size) noexcept {
  return as_writable_bytes(std::span<T, _Extent>{data, size});
}

template <typename _Type, size_t _Extent>
[[nodiscard]]
constexpr std::span<const byte,
                    _Extent == std::dynamic_extent ? std::dynamic_extent : _Extent * sizeof(_Type)>
as_bytes(std::span<_Type, _Extent> __sp) noexcept {
  auto data = reinterpret_cast<const byte*>(__sp.data());
  auto size = __sp.size_bytes();
  constexpr auto extent
      = _Extent == std::dynamic_extent ? std::dynamic_extent : _Extent * sizeof(_Type);
  return std::span<const byte, extent>{data, size};
}

template <class T, size_t _Extent = std::dynamic_extent>
[[nodiscard]]
constexpr std::span<const byte, _Extent> as_bytes(T* data, size_t size) noexcept {
  return as_bytes(std::span<T, _Extent>{data, size});
}
XSL_NE
#endif
