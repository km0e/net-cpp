#include "xsl/wheel/def.h"
#include "xsl/wheel/str.h"

WHEEL_NAMESPACE_BEGIN
void i32_to_bytes(int32_t value, char* bytes) {
  bytes[0] = (value >> 24) & 0xFF;
  bytes[1] = (value >> 16) & 0xFF;
  bytes[2] = (value >> 8) & 0xFF;
  bytes[3] = value & 0xFF;
}
int32_t i32_from_bytes(const char* bytes) {
  return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}
void bool_to_bytes(bool value, char* bytes) { bytes[0] = value ? 1 : 0; }
bool bool_from_bytes(const char* bytes) { return bytes[0] == 1; }
std::strong_ordering operator<=>(const FixedString& lhs, const FixedString& rhs) {
  return lhs.compare(std::string_view(rhs.data(), rhs.size())) <=> 0;
}
std::strong_ordering operator<=>(const FixedString& lhs, std::string_view rhs) {
  return lhs.compare(rhs) <=> 0;
}
std::strong_ordering operator<=>(const FixedString& lhs, const char* rhs) {
  return lhs.compare(std::string_view(rhs)) <=> 0;
}
WHEEL_NAMESPACE_END
