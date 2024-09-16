/**
 * @file utils.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_DNS_UTILS_H
#  define XSL_NET_DNS_UTILS_H
#  include "xsl/def.h"
#  include "xsl/net/dns/def.h"
#  include "xsl/net/dns/proto/def.h"

#  include <cstddef>
#  include <cstdint>
#  include <expected>
#  include <span>
#  include <string_view>
XSL_NET_DNS_NB

/// @brief compress the domain name
class DnCompressor {
public:
  constexpr DnCompressor(const std::uint8_t *base)
      : _src(), lens(), base(base), dnptrs(), dnptrs_cnt(0), suffix_len(0), suffix_off(0) {}
  /**
   * @brief prepare the domain name for compression
   *
   * @param src domain name, maximum length is 255. If the length is greater than 255, it will be
   * truncated
   * @return std::size_t
   */
  std::expected<std::size_t, errc> prepare(std::string_view src);
  /**
   * @brief compress the domain name
   *
   * @param dst memory to store the compressed domain name
   * @note the memory size must be greater than or equal to the size returned by the prepare method
   */
  // void compress(std::span<byte> dst);
  void compress(std::span<byte> &dst);
  constexpr void reset();

private:
  std::string_view _src;
  std::uint8_t lens[127];

  const std::uint8_t *base;

  const std::uint8_t *dnptrs[20];
  std::size_t dnptrs_cnt;

  std::size_t suffix_len;
  std::size_t suffix_off;
};

class DnDecompressor {
public:
  constexpr DnDecompressor(const byte *base) : base(base), buf(), buf_end(0) {}
  /// @brief prepare the domain name for decompression
  errc prepare(std::span<const byte> &src);
  /// @brief get the needed memory size for the decompressed domain name
  std::size_t needed() const;
  /// @brief decompress the domain name
  void decompress(std::span<byte> &src);

private:
  const byte *base;

  byte buf[size_limits::name];
  std::size_t buf_end;

  errc prepare_rest(const byte *ptr);
};
/// @brief skip the domain name, update the src
constexpr errc skip_dn(std::span<const byte> &src) {
  std::size_t offset = 0;
  while (src[offset] != 0) {
    if (src[offset] & 0xc0) {
      if (src[offset] != 0xc0) return errc::illegal_byte_sequence;
      offset += 1;
      break;
    }
    offset += src[offset] + 1;
  }
  src = src.subspan(offset + 1);
  return {};
}

XSL_NET_DNS_NE
#endif
