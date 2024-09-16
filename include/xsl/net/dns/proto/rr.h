/**
 * @file rr.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Resource Record
 * @version 0.1
 * @date 2024-09-09
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_DNS_PROTO_RR
#  define XSL_NET_DNS_PROTO_RR
#  include "xsl/def.h"
#  include "xsl/net/dns/proto/def.h"
#  include "xsl/net/dns/utils.h"
#  include "xsl/ser.h"

#  include <netinet/in.h>

#  include <expected>
#  include <memory>
XSL_NET_DNS_NB

/**
 * @brief Resource Record
 *                                  1  1  1  1  1  1
 *    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                                               |
 *  /                                               /
 *  /                      NAME                     /
 *  |                                               |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                      TYPE                     |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                     CLASS                     |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                      TTL                      |
 *  |                                               |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                   RDLENGTH                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
 *  /                     RDATA                     /
 *  /                                               /
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  Don't content the NAME field
 */
class RR {
public:
  RR(std::convertible_to<std::unique_ptr<byte[]>> auto &&data)
      : data(std::forward<decltype(data)>(data)) {}
  RR(RR &&) = default;
  RR &operator=(RR &&) = default;
  ~RR() = default;
  /// @brief get the type
  Type type() const {
    Type type;
    deserialize(data.get(), type);
    return type;
  }
  /// @brief get the class
  Class class_() const {
    Class class_;
    deserialize(data.get() + 2, class_);
    return class_;
  }
  /// @brief get the rdata length
  std::uint16_t rdlength() const {
    uint16_t u16;
    xsl::deserialize(data.get() + 8, u16);
    return ntohs(u16);
  }
  /// @brief get the rdata
  std::span<const byte> rdata() const { return {data.get() + 10, rdlength()}; }

private:
  std::unique_ptr<byte[]> data;
};
/// @brief deserialize the resource record
std::expected<std::pair<std::string, RR>, errc> deserialized(std::span<const byte> &src,
                                                                  DnDecompressor &decompressor);
XSL_NET_DNS_NE
#endif
