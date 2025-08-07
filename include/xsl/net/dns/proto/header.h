/**
 * @file header.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS protocol header
 * @version 0.2.0
 * @date 2024-08-19
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_DNS_PROTO_HEADER
#  define XSL_NET_DNS_PROTO_HEADER
#  include <netinet/in.h>
#  include <xsl/net/dns/proto/def.h>
#  include <xsl/ser.h>

#  include <cstddef>
#  include <cstdint>

XSL_NET_DNS_NB

struct Flags {
  std::uint16_t qr : 1;  // query/response
  std::uint16_t opcode : 4;
  std::uint16_t aa : 1;  // authoritative answer
  std::uint16_t tc : 1;  // truncated
  std::uint16_t rd : 1;  // recursion desired
  std::uint16_t ra : 1;  // recursion available
  std::uint16_t z : 1;
  std::uint16_t ad : 1;      // authenticated data
  std::uint16_t cd : 1;      // checking disabled
  std::uint16_t _rcode : 4;  // response code

  constexpr Flags() : qr(0), opcode(0), aa(0), tc(0), rd(0), ra(0), z(0), ad(0), cd(0), _rcode(0) {}
  constexpr Flags(std::uint16_t u16)
      : qr((u16 >> 15) & 0x1),
        opcode((u16 >> 11) & 0xf),
        aa((u16 >> 10) & 0x1),
        tc((u16 >> 9) & 0x1),
        rd((u16 >> 8) & 0x1),
        ra((u16 >> 7) & 0x1),
        z((u16 >> 6) & 0x1),
        ad((u16 >> 5) & 0x1),
        cd((u16 >> 4) & 0x1),
        _rcode(u16 & 0xf) {}
  constexpr bool operator==(const Flags &rhs) const {
    return qr == rhs.qr && opcode == rhs.opcode && aa == rhs.aa && tc == rhs.tc && rd == rhs.rd
           && ra == rhs.ra && z == rhs.z && ad == rhs.ad && cd == rhs.cd && _rcode == rhs._rcode;
  }

  /// @brief To u16
  constexpr std::uint16_t to_u16() const {
    return (qr << 15) | (opcode << 11) | (aa << 10) | (tc << 9) | (rd << 8) | (ra << 7) | (z << 6)
           | (ad << 5) | (cd << 4) | _rcode;
  }
  /// @brief Get the RCode
  constexpr RCode rcode() const { return RCode::from_u16(this->_rcode); }
  /// @brief Deserialize the flags from a buffer
  constexpr std::size_t deserialize(const byte *&buf) {
    std::uint16_t u16;
    xsl::deserialize(buf, u16);
    *this = ntohs(u16);
    return sizeof(std::uint16_t);
  }
};

/**
 * @brief DNS header
 * @see https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.1
 */
struct Header {
  static constexpr std::size_t SIZE = 12;  ///< size of the header in bytes
  std::uint16_t id;                        // transaction ID
  Flags flags;                             // flags
  std::uint16_t qdcount;                   // number of question entries
  std::uint16_t ancount;                   // number of answer entries
  std::uint16_t nscount;                   // number of authority entries
  std::uint16_t arcount;                   // number of resource entries
  /// @brief Size of the header in bytes
  consteval std::size_t size_bytes() const { return 12; }
  /// @brief Get the RCode
  constexpr RCode rcode() const { return flags.rcode(); }
  /// @brief Serialize the header, buf will be updated
  constexpr void serialize(std::span<byte> &buf) const {
    xsl::serialized(buf, id, htons(flags.to_u16()), htons(qdcount), htons(ancount), htons(nscount),
                    htons(arcount));
  }
  /// @brief Serialize the heade,
  constexpr std::size_t serialize(byte *buf) const {
    return xsl::serialize_all(buf, id, htons(flags.to_u16()), htons(qdcount), htons(ancount),
                              htons(nscount), htons(arcount));
  }
  /// @brief Deserialize the header, buf will be updated
  constexpr void deserialize(std::span<const byte> &buf) {
    xsl::deserialized(buf, id, flags, qdcount, ancount, nscount, arcount);
    this->qdcount = ntohs(this->qdcount);
    this->ancount = ntohs(this->ancount);
    this->nscount = ntohs(this->nscount);
    this->arcount = ntohs(this->arcount);
  }
  /// @brief Deserialize the header
  constexpr std::size_t deserialize(const byte *buf) {
    std::uint16_t u16;
    std::size_t sz = xsl::deserialize_all(buf, id, u16, qdcount, ancount, nscount, arcount);
    this->flags = ntohs(u16);
    this->qdcount = ntohs(this->qdcount);
    this->ancount = ntohs(this->ancount);
    this->nscount = ntohs(this->nscount);
    this->arcount = ntohs(this->arcount);
    return sz;
  }
};

struct HeaderView {
  byte *data;  ///< pointer to the header data
  constexpr HeaderView(byte *data) : data(data) {}

  /// @brief Get the question count
  constexpr std::uint16_t qdcount() const {
    std::uint16_t count;
    xsl::deserialize(data + 4, count);
    return ntohs(count);
  }
  /// @brief Set the answer Count
  constexpr void ancount(std::uint16_t count) { xsl::serialize(data + 6, htons(count)); }
  /// @brief Get the arcount
  constexpr void arcount(std::uint16_t count) { xsl::serialize(data + 10, htons(count)); }
  /// @brief Set as response
  constexpr void response() {
    Flags flags;
    xsl::deserialize(data + 2, flags);
    flags.qr = 1;  // set as response
    xsl::serialize(data + 2, htons(flags.to_u16()));
  }
  /// @brief Basic response flags set
  constexpr void basic_response() {
    Flags flags;
    xsl::deserialize(data + 2, flags);
    flags.qr = 1;  // set as response
    flags.ra = 1;  // recursion available
    flags.ad = 0;  // authenticated data
    xsl::serialize(data + 2, htons(flags.to_u16()));
  }
};
XSL_NET_DNS_NE
#endif
