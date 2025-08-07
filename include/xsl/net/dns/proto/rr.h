/**
 * @file rr.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Resource Record
 * @version 0.2.0
 * @date 2024-09-09
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_DNS_PROTO_RR
#  define XSL_NET_DNS_PROTO_RR
#  include <netinet/in.h>
#  include <xsl/def.h>
#  include <xsl/net/dns/proto/def.h>
#  include <xsl/net/dns/utils.h>
#  include <xsl/ser.h>

#  include <expected>
#  include <memory>
#  include <span>
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
 * @see https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.3
 */
const std::size_t RR_HEADER_SIZE = 10;     ///< size of the resource record header
const std::size_t RR_TYPE_OFFSET = 0;      ///< offset of the type field
const std::size_t RR_CLASS_OFFSET = 2;     ///< offset of the class field
const std::size_t RR_TTL_OFFSET = 4;       ///< offset of the ttl field
const std::size_t RR_RDLENGTH_OFFSET = 8;  ///< offset of the rdata length field
const std::size_t RR_RDATA_OFFSET = 10;    ///< offset of the rdata field

class RR {
public:
  static RR from_bytes(std::span<const byte> &src) {
    uint16_t u16;
    xsl::deserialize(src.data() + RR_RDLENGTH_OFFSET, u16);
    std::size_t rest_length = RR_HEADER_SIZE + ntohs(u16);
    if (rest_length > src.size()) {  // not enough data
      return RR{nullptr};
    }
    auto ptr = std::make_unique<byte[]>(rest_length);
    memcpy(ptr.get(), src.data(), rest_length);
    src = src.subspan(rest_length);
    return RR{std::move(ptr)};
  }

  RR(std::convertible_to<std::unique_ptr<byte[]>> auto &&data)
      : data_(std::forward<decltype(data)>(data)) {}
  RR(std::uint16_t rdlength) : data_(std::make_unique<byte[]>(RR_HEADER_SIZE + rdlength)) {
    xsl::serialize(data_.get() + RR_RDLENGTH_OFFSET, htons(rdlength));
  }
  RR(RR &&) = default;
  RR &operator=(RR &&) = default;
  ~RR() = default;
  bool is_valid() { return this->data_.get() != nullptr; }
  /// @brief get the type
  Type type() const { return Type::from_bytes(data_.get() + RR_TYPE_OFFSET); }
  /// @brief set the type
  void type(Type type) { type.serialize(data_.get() + RR_TYPE_OFFSET); }
  /// @brief get the class
  Class class_() const { return Class::from_bytes(data_.get() + RR_CLASS_OFFSET); }
  /// @brief set the class
  void class_(Class class_) { class_.serialize(data_.get() + RR_CLASS_OFFSET); }
  /// @brief get the ttl
  std::uint32_t ttl() const {
    uint32_t u32;
    xsl::deserialize(data_.get() + RR_TTL_OFFSET, u32);
    return ntohl(u32);
  }
  /// @brief set the ttl
  void ttl(std::uint32_t ttl) { xsl::serialize(data_.get() + RR_TTL_OFFSET, htonl(ttl)); }
  /// @brief get the rdata length
  std::uint16_t rdlength() const {
    uint16_t u16;
    xsl::deserialize(data_.get() + RR_RDLENGTH_OFFSET, u16);
    return ntohs(u16);
  }
  /// @brief get the rdata
  std::span<const byte> rdata() const { return {data_.get() + RR_RDATA_OFFSET, rdlength()}; }
  /// @brief set the rdata
  void rdata(const byte *rdata, std::size_t length) {
    memcpy(data_.get() + RR_RDATA_OFFSET, rdata, length);
  }
  /// @brief get the rr size
  constexpr std::size_t size() const { return RR_HEADER_SIZE + rdlength(); }
  /// @brief get the rr data
  constexpr const byte *data() const { return data_.get(); }
  /// @brief into the unique_ptr
  std::unique_ptr<byte[]> into_underlying() && {
    return std::move(data_);  // move the data out
  }

private:
  std::unique_ptr<byte[]> data_;
};

class RRView {
public:
  RRView(const byte *data) : data_(data) {}
  RRView(std::span<const byte> &data) : data_(data.data()) {
    data = data.subspan(size());  // consume the data
  }
  RRView(const RR &rr) : data_(rr.data()) {}
  RRView(const RRView &) = default;
  RRView &operator=(const RRView &) = default;
  RRView(RRView &&) = default;
  RRView &operator=(RRView &&) = default;
  ~RRView() = default;

  /// @brief get the type
  Type type() const { return Type::from_bytes(data_ + RR_TYPE_OFFSET); }
  /// @brief get the class
  Class class_() const { return Class::from_bytes(data_ + RR_CLASS_OFFSET); }
  /// @brief get the ttl
  std::uint32_t ttl() const {
    uint32_t u32;
    xsl::deserialize(data_ + RR_TTL_OFFSET, u32);
    return ntohl(u32);
  }
  /// @brief get the rdata length
  std::uint16_t rdlength() const {
    uint16_t u16;
    xsl::deserialize(data_ + RR_RDLENGTH_OFFSET, u16);
    return ntohs(u16);
  }
  /// @brief get the rdata
  const byte *rdata() const {
    return data_ + RR_RDATA_OFFSET;  // return the pointer to the rdata
  }

  /// @brief get the rr size
  constexpr std::size_t size() const { return RR_HEADER_SIZE + rdlength(); }
  /// @brief get the rr data
  constexpr const byte *data() const { return data_; }

private:
  const byte *data_;  ///< pointer to the resource record data
};

/// @brief deserialize the resource record
std::expected<std::pair<std::string, RR>, errc> deserialized(std::span<const byte> &src,
                                                             DnDecompressor &decompressor);

class RRSerializer {
private:
  std::span<byte> &buf;

public:
  RRSerializer(std::span<byte> &buf) : buf(buf) {}
  RRSerializer &type(Type type) {
    type.serialized(buf);
    return *this;
  }
  RRSerializer &class_(Class class_) {
    class_.serialized(buf);
    return *this;
  }
  RRSerializer &ttl(std::uint32_t ttl) {
    xsl::serialized(buf, htonl(ttl));
    return *this;
  }
  RRSerializer &rdata(std::span<const byte> rdata) {
    xsl::serialized(buf, htons(rdata.size()));
    memcpy(buf.data(), rdata.data(), rdata.size());
    buf = buf.subspan(rdata.size());
    return *this;
  }
};

XSL_NET_DNS_NE
#endif
