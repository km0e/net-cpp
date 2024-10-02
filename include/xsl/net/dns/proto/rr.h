/**
 * @file rr.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Resource Record
 * @version 0.11
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
  static RR from_bytes(std::span<const byte>&src) {
    uint16_t u16;
    xsl::deserialize(src.data() + 8, u16);
    std::size_t rest_length = 2 + 2 + 4 + 2 + ntohs(u16);
    if (rest_length > src.size()) {  // not enough data
      return RR{nullptr};
    }
    auto ptr = std::make_unique<byte[]>(rest_length);
    memcpy(ptr.get(), src.data(), rest_length);
    src = src.subspan(rest_length);
    return RR{std::move(ptr)};
  }

  RR(std::convertible_to<std::unique_ptr<byte[]>> auto &&data)
      : data(std::forward<decltype(data)>(data)) {}
  RR(RR &&) = default;
  RR &operator=(RR &&) = default;
  ~RR() = default;
  bool is_valid() { return this->data.get() != nullptr; }
  /// @brief get the type
  Type type() const { return Type::from_bytes(data.get()); }
  /// @brief get the class
  Class class_() const { return Class::from_bytes(data.get() + 2); }
  /// @brief get the ttl
  std::uint32_t ttl() const {
    uint32_t u32;
    xsl::deserialize(data.get() + 4, u32);
    return ntohl(u32);
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
