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

#  include <cstddef>
#  include <cstdint>
#  include <string_view>

XSL_NET_DNS_NB

const std::string_view RCODE_STR[]
    = {"No Error", "Format Error", "Server Failure", "Name Error", "Not Implemented", "Refused"};

/**
 * @brief RCodes
 * @see https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.1
 */
struct RCode {
  enum : std::uint8_t {
    NO_ERROR = 0,         // No error condition -> errc::success
    FORMAT_ERROR = 1,     // Format error -> errc::invalid_argument
    SERVER_FAILURE = 2,   // Server failure -> errc::io_error
    NAME_ERROR = 3,       // Name Error -> errc::no_such_file_or_directory
    NOT_IMPLEMENTED = 4,  // Not Implemented -> errc::function_not_supported
    REFUSED = 5,          // Refused -> errc::operation_not_permitted
  } _code;

  static constexpr RCode from_u16(std::uint16_t u16) { return {static_cast<decltype(_code)>(u16)}; }

  /// @brief Map RCode to errc
  constexpr errc to_errc() const {
    switch (_code) {
      case NO_ERROR:
        return errc{};
      case FORMAT_ERROR:
        return errc::invalid_argument;
      case SERVER_FAILURE:
        return errc::io_error;
      case NAME_ERROR:
        return errc::no_such_file_or_directory;
      case NOT_IMPLEMENTED:
        return errc::function_not_supported;
      case REFUSED:
        return errc::operation_not_permitted;
      default:
        return errc::protocol_error;
    }
  }

  /// @brief To string_view
  constexpr std::string_view to_string_view() const { return RCODE_STR[_code]; }
};

constexpr bool operator==(const RCode &lhs, const decltype(RCode::_code) &rhs) {
  return lhs._code == rhs;
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
const std::string_view TYPE_STR[]
    = {"A",   "NS",  "MD",    "MF",    "CNAME", "SOA", "MB",   "MG",    "MR",    "NULL",
       "WKS", "PTR", "HINFO", "MINFO", "MX",    "TXT", "AXFR", "MAILB", "MAILA", "ANY"};
const std::size_t MAX_CONSECUTIVE_TYPE_INDEX = 16;

struct Type {
  enum : std::uint16_t {
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
  } _type;

  constexpr Type() = default;
  constexpr Type(decltype(_type) type) : _type(type) {}

  static constexpr Type from_bytes(const byte *buf) {
    std::uint16_t u16;
    xsl::deserialize(buf, u16);
    return {static_cast<decltype(_type)>(ntohs(u16))};
  }

  /// @brief Map Type to string_view
  constexpr std::string_view to_string_view() const {
    if (_type < 1)
      return "Unknown";
    else if (_type < MAX_CONSECUTIVE_TYPE_INDEX)
      return TYPE_STR[_type - 1];
    else if (_type < 252)
      return "Unknown";
    else if (_type < 256)
      return TYPE_STR[_type - 252 + MAX_CONSECUTIVE_TYPE_INDEX];
    else
      return "Unknown";
  }
  /// @brief Serialize Type to network byte order
  constexpr void serialized(std::span<byte> &buf) const { xsl::serialized(buf, htons(_type)); }
  /// @brief Deserialize Type
  constexpr void deserialize(const byte *buf) {
    std::uint16_t u16;
    xsl::deserialize(buf, u16);
    _type = static_cast<decltype(_type)>(ntohs(u16));
  }
};

constexpr bool operator==(const Type &lhs, const decltype(Type::_type) &rhs) {
  return lhs._type == rhs;
}

constexpr bool operator==(const Type &lhs, const Type &rhs) { return lhs == rhs._type; }

const std::string_view CLASS_STR[] = {"IN", "CS", "CH", "HS", "ANY"};
const std::size_t MAX_CONSECUTIVE_CLASS_INDEX = 4;
struct Class {
  enum : std::uint16_t {
    IN = 1,  // the Internet
    CS = 2,  // the CSNET class (Obsolete - used only for examples in some obsolete RFCs)
    CH = 3,  // the CHAOS class
    HS = 4,  // Hesiod [Dyer 87]

    /// Query only
    ANY = 255,  // any class
  } _class;

  constexpr Class() = default;
  constexpr Class(decltype(_class) class_) : _class(class_) {}

  static constexpr Class from_bytes(const byte *buf) {
    std::uint16_t u16;
    xsl::deserialize(buf, u16);
    return {static_cast<decltype(_class)>(ntohs(u16))};
  }

  /// @brief Map Class to string_view
  constexpr std::string_view to_string_view() const {
    if (_class < 1)
      return "Unknown";
    else if (_class < MAX_CONSECUTIVE_CLASS_INDEX)
      return CLASS_STR[_class - 1];
    else if (_class < 255)
      return "Unknown";
    else
      return CLASS_STR[MAX_CONSECUTIVE_CLASS_INDEX];
  }

  /// @brief Serialize Class to network byte order
  constexpr void serialized(std::span<byte> &buf) const { xsl::serialized(buf, htons(_class)); }
  /// @brief Deserialize Class
  constexpr void deserialize(const byte *buf) {
    std::uint16_t u16;
    xsl::deserialize(buf, u16);
    _class = static_cast<decltype(_class)>(ntohs(u16));
  }
};

constexpr bool operator==(const Class &lhs, const decltype(Class::_class) &rhs) {
  return lhs._class == rhs;
}

constexpr bool operator==(const Class &lhs, const Class &rhs) { return lhs == rhs._class; }

XSL_NET_DNS_NE
#endif
