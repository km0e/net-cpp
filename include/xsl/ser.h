/**
 * @file ser.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Serialization and deserialization utilities
 * @version 0.2.0
 * @date 2024-09-11
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SER
#  define XSL_SER
#  include <xsl/byte.h>
#  include <xsl/def.h>

#  include <concepts>
#  include <cstring>
#  include <span>
XSL_NB

template <typename I>
constexpr std::size_t serialize(byte* buf, const I& value) {
  if constexpr (requires { value.serialize(buf); }) {
    return value.serialize(buf);
  }
  auto raw = std::as_bytes(std::span(&value, 1));
  std::copy(raw.begin(), raw.end(), buf);
  return sizeof(I);
}

template <typename... Args>
constexpr std::size_t serialize_all(byte* buf, Args&&... args) {
  std::size_t offset = 0;
  ((offset += serialize(buf + offset, std::forward<Args>(args))), ...);
  return offset;
}

template <typename... Args>
constexpr void serialized(std::span<byte>& buf, Args&&... args) {
  auto offset = serialize_all(buf.data(), std::forward<Args>(args)...);
  buf = buf.subspan(offset);
}

template <std::integral T>
constexpr T* reserved(std::span<byte>& buf) {
  auto ptr = reinterpret_cast<T*>(buf.data());
  buf = buf.subspan(sizeof(T));
  return ptr;
}

template <typename T>
constexpr std::size_t deserialize(const byte* buf, T& value) {
  if constexpr (requires { value.deserialize(buf); }) {
    return value.deserialize(buf);
  }
  std::copy(buf, buf + sizeof(T), std::as_writable_bytes(std::span(&value, 1)).begin());
  return sizeof(T);
}

template <typename... Args>
constexpr std::size_t deserialize_all(const byte* buf, Args&&... args) {
  std::size_t offset = 0;
  ((offset += deserialize(buf + offset, std::forward<Args>(args))), ...);
  return offset;
}

template <typename... Args>
constexpr void deserialized(std::span<const byte>& buf, Args&&... args) {
  std::size_t offset = deserialize_all(buf.data(), std::forward<Args>(args)...);
  buf = buf.subspan(offset);
}

XSL_NE
#endif
