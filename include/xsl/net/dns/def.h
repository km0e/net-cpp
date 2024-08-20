#pragma once
#include "xsl/wheel/str.h"

#include <netinet/in.h>
#ifndef XSL_NET_DNS_DEF
#  define XSL_NET_DNS_DEF
#  define XSL_NET_DNS_NB namespace xsl::_net::dns {
#  define XSL_NET_DNS_NE }
#  include <cstddef>
#  include <cstdint>
XSL_NET_DNS_NB
/**
 * @brief size limits of DNS
 * @ref https://datatracker.ietf.org/doc/html/rfc1035#section-2.3.4
 */
namespace size_limits {
  // labels          63 octets or less
  const std::size_t label = 63;
  // names           255 octets or less
  const std::size_t name = 255;
  // TTL             32 bit unsigned integer
  const std::size_t ttl = 4;
  // UDP messages    512 octets or less
  const std::size_t udp_message = 512;
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
};
enum class ExtQType : std::uint16_t {
  AXFR = 252,   // A request for a transfer of an entire zone
  MAILB = 253,  // A request for mailbox-related records (MB, MG or MR)
  MAILA = 254,  // A request for mail agent RRs (Obsolete - see MX)
  ANY = 255,    // A request for all records
};
inline void serialize(Type value, byte *buf) {
  wheel::u16_to_bytes(htons(static_cast<std::uint16_t>(value)), buf);
}
inline void serialize(ExtQType value, byte *buf) {
  wheel::u16_to_bytes(htons(static_cast<std::uint16_t>(value)), buf);
}
enum class Class : std::uint16_t {
  IN = 1,  // the Internet
  CS = 2,  // the CSNET class (Obsolete - used only for examples in some obsolete RFCs)
  CH = 3,  // the CHAOS class
  HS = 4,  // Hesiod [Dyer 87]
};
enum class ExtQClass : std::uint16_t {
  ANY = 255,  // any class
};
inline void serialize(Class value, byte *buf) {
  wheel::u16_to_bytes(htons(static_cast<std::uint16_t>(value)), buf);
}
inline void serialize(ExtQClass value, byte *buf) {
  wheel::u16_to_bytes(htons(static_cast<std::uint16_t>(value)), buf);
}
XSL_NET_DNS_NE
#endif
