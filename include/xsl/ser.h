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
#  include "xsl/def.h"
#  include "xsl/wheel.h"

#  include <cstring>
#  include <span>
XSL_NB


constexpr void serialize(byte* buf, int32_t value) {
  auto raw = xsl::as_bytes(std::span(&value, 1));
  std::copy(raw.begin(), raw.end(), buf);
}

constexpr void serialized(std::span<byte>& buf, int32_t value) {
  serialize(buf.data(), value);
  buf = buf.subspan(sizeof(int32_t));
}

constexpr void serialize(byte* buf, uint16_t value) {
  auto raw = xsl::as_bytes(std::span(&value, 1));
  std::copy(raw.begin(), raw.end(), buf);
}

constexpr void serialized(std::span<byte>& buf, uint16_t value) {
  serialize(buf.data(), value);
  buf = buf.subspan(sizeof(uint16_t));
}

constexpr void deserialize(const byte* buf, int32_t& value) {
  std::copy(buf, buf + sizeof(int32_t), wheel::as_writable_bytes(std::span(&value, 1)).begin());
}

constexpr void deserialized(std::span<const byte>& buf, int32_t& value) {
  deserialize(buf.data(), value);
  buf = buf.subspan(sizeof(int32_t));
}

constexpr void deserialize(const byte* buf, uint16_t& value) {
  std::copy(buf, buf + sizeof(uint16_t), wheel::as_writable_bytes(std::span(&value, 1)).begin());
}

constexpr void deserialized(std::span<const byte>& buf, uint16_t& value) {
  deserialize(buf.data(), value);
  buf = buf.subspan(sizeof(uint16_t));
}

template <typename... Args>
constexpr void serialized(std::span<byte>& buf, Args&&... args) {
  (serialized(buf, args), ...);
}

template <typename... Args>
constexpr void deserialized(std::span<const byte>& buf, Args&&... args) {
  (deserialized(buf, args), ...);
}

XSL_NE
#endif
