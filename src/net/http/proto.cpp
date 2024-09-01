/**
 * @file proto.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/net/http/def.h"
#include "xsl/net/http/proto.h"

XSL_HTTP_NB

constexpr std::string_view to_string_view(const Method& method) {
  if (method == Method::UNKNOWN) return "Unknown";
  return HTTP_METHOD_STRINGS[static_cast<uint8_t>(method)];
}

constexpr std::string_view to_string_view(const Charset& charset) {
  if (charset == Charset::UNKNOWN) return "Unknown";
  return CHARSET_STRINGS[static_cast<uint8_t>(charset)];
}

XSL_HTTP_NE
