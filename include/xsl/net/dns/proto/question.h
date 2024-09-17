/**
 * @file question.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_DNS_PROTO_QUESTION
#  define XSL_NET_DNS_PROTO_QUESTION
#  include "xsl/net/dns/proto/def.h"
#  include "xsl/net/dns/utils.h"

#  include <expected>
#  include <string_view>

XSL_NET_DNS_NB
/**
 * @brief serialize the question
 *
 * @param buf buffer to store the serialized data
 * @param dn domain name
 * @param type type
 * @param class_ class
 * @param compressor domain name compressor
 * @return errc
 */
constexpr errc serialized(std::span<byte> &buf, const std::string_view &dn, Type type,
                               Class class_, DnCompressor &compressor) {
  auto status = compressor.prepare(dn);
  if (!status) {
    return status.error();
  }
  compressor.compress(buf);
  serialized(buf, type);
  serialized(buf, class_);
  return {};
}
/// @brief skip the question part
constexpr errc skip_question(std::span<const byte> &src) {
  auto ec = skip_dn(src);
  if (ec != errc{}) {
    return ec;
  }
  src = src.subspan(4);
  return {};
}

XSL_NET_DNS_NE
#endif
