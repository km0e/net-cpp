/**
 * @file ser.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Serialization and deserialization utilities
 * @version 0.1
 * @date 2024-09-11
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SER
#  define XSL_SER
#  include "xsl/byte.h"
#  include "xsl/def.h"

#  include <concepts>
#  include <cstring>
#  include <span>
XSL_NB

template <std::integral I>
constexpr void serialize(byte* buf, I value) {
  auto raw = std::as_bytes(std::span(&value, 1));
  std::copy(raw.begin(), raw.end(), buf);
}

template <std::integral T, size_t _Extent = std::dynamic_extent>
constexpr void serialized(std::span<byte, _Extent>& buf, T value) {
  serialize(buf.data(), value);
  buf = buf.subspan(sizeof(T));
}

template <std::integral T>
constexpr T* reserved(std::span<byte>& buf) {
  auto ptr = reinterpret_cast<T*>(buf.data());
  buf = buf.subspan(sizeof(T));
  return ptr;
}

template <typename T>
constexpr void deserialize(const byte* buf, T& value) {
  std::copy(buf, buf + sizeof(T), std::as_writable_bytes(std::span(&value, 1)).begin());
}

template <typename T, size_t _Extent = std::dynamic_extent>
constexpr void deserialized(std::span<const byte, _Extent>& buf, T& value) {
  deserialize(buf.data(), value);
  buf = buf.subspan(sizeof(T));
}

template <typename... Args>
constexpr void serialized_all(std::span<byte>& buf, Args&&... args) {
  (serialized(buf, args), ...);
}

template <typename... Args>
constexpr void deserialized_all(std::span<const byte>& buf, Args&&... args) {
  (deserialized(buf, args), ...);
}

XSL_NE
#endif
