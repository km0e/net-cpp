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
#ifndef XSL_NET_DNS_PROTO_HEADER
#  define XSL_NET_DNS_PROTO_HEADER
#  include "xsl/def.h"
#  include "xsl/net/dns/proto/def.h"
#  include "xsl/ser.h"

#  include <netinet/in.h>

#  include <cstddef>
#  include <cstdint>

XSL_NET_DNS_NB
/**
 * @brief DNS header
 * @see https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.1
 */
struct Header {
  std::uint16_t id;       // transaction ID
  std::uint16_t flags;    // flags
  std::uint16_t qdcount;  // number of question entries
  std::uint16_t ancount;  // number of answer entries
  std::uint16_t nscount;  // number of authority entries
  std::uint16_t arcount;  // number of resource entries
  /// @brief Size of the header in bytes
  consteval std::size_t size_bytes() const { return 12; }
  /// @brief Get the RCode
  constexpr RCode rcode() const { return static_cast<RCode>(flags & 0b0000'0000'0000'1111); }
  /// @brief Serialize the header, buf will be updated
  constexpr void serialize(std::span<byte> &buf) const {
    xsl::serialized(buf, id, htons(flags), htons(qdcount), htons(ancount), htons(nscount),
                    htons(arcount));
  }
  /// @brief Deserialize the header, buf will be updated
  constexpr void deserialize(std::span<const byte> &buf) {
    xsl::deserialized(buf, id, flags, qdcount, ancount, nscount, arcount);
    this->flags = ntohs(this->flags);
    this->qdcount = ntohs(this->qdcount);
    this->ancount = ntohs(this->ancount);
    this->nscount = ntohs(this->nscount);
    this->arcount = ntohs(this->arcount);
  }
};
XSL_NET_DNS_NE
#endif
