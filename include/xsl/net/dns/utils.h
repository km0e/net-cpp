/**
 * @file utils.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.12
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

#  include <cassert>
#  include <cstddef>
#  include <cstdint>
#  include <expected>
#  include <span>
#  include <string_view>
XSL_NET_DNS_NB

/// @brief compress the domain name
class DnCompressor {
public:
  constexpr DnCompressor(const byte *base)
      : _src(),
        lens(),
        base(reinterpret_cast<const uint8_t *>(base)),
        dnptrs(),
        dnptrs_cnt(0),
        suffix_len(0),
        suffix_off(0) {}
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
  void compress(std::span<byte> &_dst) {
    std::span<uint8_t> dst(reinterpret_cast<uint8_t *>(_dst.data()), _dst.size());
    if (_src.empty()) {
      *dst.data() = 0;
      _dst = _dst.subspan(1);
      return;
    }
    assert(dst.size() > 0
           && dst.size()
                  >= _src.size() - suffix_len + 2 + (0 < suffix_len && suffix_len < _src.size()));
    memcpy(dst.data() + 1, _src.data(), _src.size() - suffix_len);
    std::size_t i = 0;
    for (std::size_t j = 0; i < _src.size() - suffix_len; j++) {
      dst[i] = lens[j];
      i += lens[j] + 1;  // jump to the next label length field
    }
    if (suffix_len) {
      dst[i++] = 0xc0 | suffix_off >> 8;  // high 2 bits should be 11
    }
    dst[i++] = suffix_off;  // low 8 bits or 0 if suffix_len is 0

    if (i > 2) {
      dnptrs[dnptrs_cnt] = dst.data();  // store the pointer
      dnptrs_cnt++;                     // increase the pointer count
    }
    _dst = _dst.subspan(i);  // i is the size of the compressed domain name
    this->reset();
  }
  constexpr void reset() {
    _src = {};
    suffix_len = 0;
    suffix_off = 0;
  }

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
  constexpr DnDecompressor(const byte *base)
      : base(reinterpret_cast<const uint8_t *>(base)), buf(), buf_end{} {}
  /// @brief prepare the domain name for decompression
  errc decompress(std::span<const byte> &src);
  /// @brief get the decompressed domain name
  std::string_view dn() const;
  /// @brief get the needed memory size for the decompressed domain name
  std::size_t needed() const;
  /// @brief decompress the domain name

private:
  const uint8_t *base;

  uint8_t buf[size_limits::name];
  std::size_t buf_end;

  errc prepare_rest(const uint8_t *ptr);
};
/// @brief skip the domain name, update the src
constexpr errc skip_dn(std::span<const byte> &src_) {
  std::span<const uint8_t> src(reinterpret_cast<const uint8_t *>(src_.data()), src_.size());
  std::size_t offset = 0;
  while (src[offset] != 0) {
    if (src[offset] & 0xc0) {
      if (src[offset] != 0xc0) return errc::illegal_byte_sequence;
      offset += 1;
      break;
    }
    offset += src[offset] + 1;
  }
  src_ = src_.subspan(offset + 1);
  return {};
}

XSL_NET_DNS_NE
#endif
