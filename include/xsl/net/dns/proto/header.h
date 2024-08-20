/**
 * @file header.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-19
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_DNS_PROTO_HEADER_H
#  define XSL_NET_DNS_PROTO_HEADER_H
#  include "xsl/def.h"
#  include "xsl/net/dns/def.h"

#  include <cstdint>

XSL_NET_DNS_NB
/**
 * @brief DNS header
 * @ref https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.1
 */
struct Header {
  std::uint16_t id;       // transaction ID
  std::uint16_t flags;    // flags
  std::uint16_t qdcount;  // number of question entries
  std::uint16_t ancount;  // number of answer entries
  std::uint16_t nscount;  // number of authority entries
  std::uint16_t arcount;  // number of resource entries
  void serialize(byte *buf) const {
    buf[0] = id >> 8;
    buf[1] = id & 0xff;
    buf[2] = flags >> 8;
    buf[3] = flags & 0xff;
    buf[4] = qdcount >> 8;
    buf[5] = qdcount & 0xff;
    buf[6] = ancount >> 8;
    buf[7] = ancount & 0xff;
    buf[8] = nscount >> 8;
    buf[9] = nscount & 0xff;
    buf[10] = arcount >> 8;
    buf[11] = arcount & 0xff;
  }
};
XSL_NET_DNS_NE
#endif
