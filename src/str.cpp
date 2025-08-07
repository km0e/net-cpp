/**
 * @file str.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <xsl/def.h>
#include <xsl/str.h>

XSL_NB

constexpr std::strong_ordering operator<=>(const FixedString& lhs, const FixedString& rhs) {
  return lhs.compare(std::string_view(rhs.data(), rhs.size())) <=> 0;
}
constexpr std::strong_ordering operator<=>(const FixedString& lhs, std::string_view rhs) {
  return lhs.compare(rhs) <=> 0;
}
constexpr std::strong_ordering operator<=>(const FixedString& lhs, const char* rhs) {
  return lhs.compare(std::string_view(rhs)) <=> 0;
}
XSL_NE
