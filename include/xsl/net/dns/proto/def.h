/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS protocol definitions
 * @version 0.1
 * @date 2024-09-09
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_DNS_PROTO_DEF
#  define XSL_NET_DNS_PROTO_DEF
#  include "xsl/def.h"
#  include "xsl/net/dns/def.h"
#  include "xsl/ser.h"

#  include <netinet/in.h>

#  include <cstdint>

XSL_NET_DNS_NB
/**
 * @brief RCodes
 * @see https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.1
 */
enum class RCode : std::uint8_t {
  NO_ERROR = 0,         // No error condition -> std::errc::success
  FORMAT_ERROR = 1,     // Format error -> std::errc::invalid_argument
  SERVER_FAILURE = 2,   // Server failure -> std::errc::io_error
  NAME_ERROR = 3,       // Name Error -> std::errc::no_such_file_or_directory
  NOT_IMPLEMENTED = 4,  // Not Implemented -> std::errc::function_not_supported
  REFUSED = 5,          // Refused -> std::errc::operation_not_permitted
};
/// @brief Map RCode to std::errc
constexpr std::errc map_errc(RCode rcode) {
  switch (rcode) {
    case RCode::NO_ERROR:
      return std::errc{};
    case RCode::FORMAT_ERROR:
      return std::errc::invalid_argument;
    case RCode::SERVER_FAILURE:
      return std::errc::io_error;
    case RCode::NAME_ERROR:
      return std::errc::no_such_file_or_directory;
    case RCode::NOT_IMPLEMENTED:
      return std::errc::function_not_supported;
    case RCode::REFUSED:
      return std::errc::operation_not_permitted;
    default:
      return std::errc::protocol_error;
  }
}

/**
 * @brief size limits of DNS
 * @see https://datatracker.ietf.org/doc/html/rfc1035#section-2.3.4
 */
namespace size_limits {
  const std::size_t label = 63;         ///< labels, 63 octets or less
  const std::size_t name = 255;         ///< names, 255 octets or less
  const std::size_t ttl = 4;            ///< TTL, 32 bit unsigned integer
  const std::size_t udp_message = 512;  ///< UDP messages, 512 octets or less
}  // namespace size_limits
enum class Type : std::uint16_t {
  A = 1,                  // a host address
  NS = 2,                 // an authoritative name server
  MD [[deprecated]] = 3,  // a mail destination (Obsolete - use MX)
  MF [[deprecated]] = 4,  // a mail forwarder (Obsolete - use MX)
  CNAME = 5,              // the canonical name for an alias
  SOA = 6,                // marks the start of a zone of authority
  MB = 7,                 // a mailbox domain name (EXPERIMENTAL)
  MG = 8,                 // a mail group member (EXPERIMENTAL)
  MR = 9,                 // a mail rename domain name (EXPERIMENTAL)
  _NULL = 10,             // a null RR (EXPERIMENTAL)
  WKS = 11,               // a well known service description
  PTR = 12,               // a domain name pointer
  HINFO = 13,             // host information
  MINFO = 14,             // mailbox or mail list information
  MX = 15,                // mail exchange
  TXT = 16,               // text strings

  /// Query only
  AXFR = 252,   // A request for a transfer of an entire zone
  MAILB = 253,  // A request for mailbox-related records (MB, MG or MR)
  MAILA = 254,  // A request for mail agent RRs (Obsolete - see MX)
  ANY = 255,    // A request for all records
};
/// @brief Serialize Type
constexpr void serialized(std::span<byte> &buf, Type value) {
  xsl::serialized(buf, htons(static_cast<std::uint16_t>(value)));
}
/// @brief Deserialize Type
constexpr void deserialize(const byte *buf, Type &value) {
  std::uint16_t u16;
  xsl::deserialize(buf, u16);
  value = static_cast<Type>(ntohs(u16));
}
enum class Class : std::uint16_t {
  IN = 1,  // the Internet
  CS = 2,  // the CSNET class (Obsolete - used only for examples in some obsolete RFCs)
  CH = 3,  // the CHAOS class
  HS = 4,  // Hesiod [Dyer 87]

  /// Query only
  ANY = 255,  // any class
};
/// @brief Serialize Class
constexpr void serialized(std::span<byte> &buf, Class value) {
  xsl::serialized(buf, htons(static_cast<std::uint16_t>(value)));
}
/// @brief Deserialize Class
constexpr void deserialize(const byte *buf, Class &value) {
  std::uint16_t u16;
  xsl::deserialize(buf, u16);
  value = static_cast<Class>(ntohs(u16));
}
XSL_NET_DNS_NE
#endif
