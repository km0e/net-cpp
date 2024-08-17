#include "xsl/wheel/def.h"
#include "xsl/wheel/str.h"
#include "xsl/wheel/utils.h"

#include <algorithm>

XSL_WHEEL_NB

void i32_to_bytes(int32_t value, std::span<byte> bytes) {
  // bytes[0] = (value >> 24) & 0xFF;
  // bytes[1] = (value >> 16) & 0xFF;
  // bytes[2] = (value >> 8) & 0xFF;
  // bytes[3] = value & 0xFF;

  auto raw = wheel::as_bytes(std::span(&value, 1));
  std::copy(raw.begin(), raw.end(), bytes.begin());
}

int32_t i32_from_bytes(std::span<const byte> bytes) {
  int32_t value;
  std::copy(bytes.begin(), bytes.begin() + sizeof(int32_t),
            wheel::as_writable_bytes(std::span(&value, 1)).begin());
  return value;
}

// void bool_to_bytes(bool value, byte* bytes) { bytes[0] = value ? 1 : 0; }

// bool bool_from_bytes(const byte* bytes) { return bytes[0] == 1; }

std::strong_ordering operator<=>(const FixedString& lhs, const FixedString& rhs) {
  return lhs.compare(std::string_view(rhs.data(), rhs.size())) <=> 0;
}
std::strong_ordering operator<=>(const FixedString& lhs, std::string_view rhs) {
  return lhs.compare(rhs) <=> 0;
}
std::strong_ordering operator<=>(const FixedString& lhs, const char* rhs) {
  return lhs.compare(std::string_view(rhs)) <=> 0;
}
XSL_WHEEL_NE
