/**
 * @file utils.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief RFC 1035 message compression
 * @version 0.1
 * @date 2024-08-19
 * @ref https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.4
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/net/dns/def.h"
#include "xsl/net/dns/utils.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <span>
#include <system_error>
XSL_NET_DNS_NB
/**
 * @brief Get all the label start offsets of a compressed domain name
 *
 * @param offs label start offsets
 * @param base base pointer
 * @param dn domain name
 * @return std::size_t
 */
static std::size_t get_offs(short *offs, const std::uint8_t *base, const std::uint8_t *dn) {
  std::size_t noff = 0;
  for (;;) {
    while (*dn & 0xc0) {                   // jump to the label if it is a pointer
      if ((*dn & 0xc0) != 0xc0) return 0;  // invalid pointer, high 2 bits should be 11
      dn = base + ((dn[0] & 0x3f) << 8 | dn[1]);
    }
    if (*dn == 0) return noff;
    assert(dn - base < 0x4000);
    offs[noff] = dn - base;
    noff++;
    dn += *dn + 1;
  }
}
//  * @brief label lengths of an ascii domain name s

/**
 * @brief Get the label lengths of a domain name
 *
 * @param lens label lengths
 * @param dn domain name
 * @return std::size_t
 */
static std::size_t get_label_lens(std::uint8_t *lens, const std::string_view &dn) {
  for (std::size_t label_cnt = 0, label_start = 0;;) {
    auto label_end = dn.find('.', label_start);
    if (label_end == std::string_view::npos) label_end = dn.size();
    if (label_end - label_start > size_limits::label) return 0;  // 63 is the maximum label lengths
    lens[label_cnt] = label_end - label_start;
    label_cnt++;
    if (label_end == dn.size()) return label_cnt;  // end of the domain name
    label_start = label_end + 1;
  }
}
/**
 * @brief Match the domain name with the compressed domain name
 *
 * @param offs label start offsets
 * @param noff number of label start offsets
 * @param offset offset of the matched label
 * @param base base pointer
 * @param end end of the domain name
 * @param lens label lengths
 * @param nlen number of label lengths
 * @return std::size_t
 */
static std::size_t match(short *offs, int noff, int *offset, const std::uint8_t *base,
                         const char *end, const std::uint8_t *lens, int nlen) {
  int len, off;
  std::size_t m = 0;
  for (;;) {
    len = lens[--nlen];  // get the length of the last label
    off = offs[--noff];
    end -= len;
    if (len != base[off] || std::memcmp(base + off + 1, end, len)) return m;
    *offset = off;
    m += len;
    if (nlen) m++;
    if (!nlen || !noff) return m;
    end--;
  }
}
/**
 * @brief Prepare the domain name for compression
 *
 * @param src domain name
 * @return Status
 */
[[nodiscard("must check the return value")]]
std::expected<std::size_t, errc> DnCompressor::prepare(std::string_view src) {
  if (src.empty() || src.size() > size_limits::name)
    return std::unexpected{errc::invalid_argument};  /// invalid domain name
  if (src.back() == '.') src.remove_suffix(1);
  if (src.empty()) return {1};
  auto label_cnt = get_label_lens(lens, src);
  if (!label_cnt) return std::unexpected{errc::invalid_argument};  /// invalid domain name

  for (auto p : std::span{dnptrs, dnptrs_cnt}) {
    short offs[128];
    int noff = get_offs(offs, *dnptrs, p);
    if (!noff) return std::unexpected{errc::illegal_byte_sequence};  // invalid pointer
    auto offset = 0;
    auto m = match(offs, noff, &offset, *dnptrs, src.data() + src.size(), lens, label_cnt);
    if (m > suffix_len) {
      suffix_len = m;
      suffix_off = offset;
      if (m == src.size()) break;
    }
  }
  this->_src = src;
  return {src.size() - suffix_len + 2 + (0 < suffix_len && suffix_len < src.size())};
}
/**
 * @brief Compress the domain name
 *
 * @param dst
 */
// void DnCompressor::compress(std::span<byte> dst) {
//   if (_src.empty()) {
//     *dst.data() = 0;
//     return;
//   }
//   assert(dst.size() > 0
//          && dst.size()
//                 >= _src.size() - suffix_len + 2 + (0 < suffix_len && suffix_len < _src.size()));
//   memcpy(dst.data() + 1, _src.data(), _src.size() - suffix_len);
//   std::size_t i = 0;
//   for (std::size_t j = 0; i < _src.size() - suffix_len; j++) {
//     dst[i] = lens[j];
//     i += lens[j] + 1;  // jump to the next label length field
//   }
//   if (suffix_len) {
//     dst[i++] = 0xc0 | suffix_off >> 8;  // high 2 bits should be 11
//   }
//   dst[i++] = suffix_off;  // low 8 bits or 0 if suffix_len is 0

//   if (i > 2) {
//     dnptrs[dnptrs_cnt] = dst.data();  // store the pointer
//     dnptrs_cnt++;                     // increase the pointer count
//   }
//   this->reset();
// }
void DnCompressor::compress(std::span<byte> &dst) {
  if (_src.empty()) {
    *dst.data() = 0;
    dst = dst.subspan(1);
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
  dst = dst.subspan(i);  // i is the size of the compressed domain name
  this->reset();
}
constexpr void DnCompressor::reset() {
  _src = {};
  suffix_len = 0;
  suffix_off = 0;
}

errc DnDecompressor::prepare(std::span<const byte> &src) {
  const byte *ptr = src.data();
  for (;;) {
    src = src.subspan(1);
    if (*ptr == 0) return {};
    while (*ptr & 0xc0) {  // jump to the label if it is a pointer
      if ((*ptr & 0xc0) != 0xc0) return errc::illegal_byte_sequence;  // invalid pointer
      ptr = this->base + ((ptr[0] & 0x3f) << 8 | ptr[1]);
      src = src.subspan(1);
      return this->prepare_rest(ptr);
    }
    assert(ptr - base < 0x4000);
    memcpy(this->buf + this->buf_end, ptr + 1, *ptr);
    this->buf[this->buf_end + *ptr] = '.';
    this->buf_end += *ptr + 1;
    src = src.subspan(*ptr);
    ptr += *ptr + 1;
  }
}
std::size_t DnDecompressor::needed() const { return buf_end; }

errc DnDecompressor::prepare_rest(const byte *ptr) {
  for (;;) {
    if (*ptr == 0) return {};
    while (*ptr & 0xc0) {  // jump to the label if it is a pointer
      if ((*ptr & 0xc0) != 0xc0) return errc::illegal_byte_sequence;  // invalid pointer
      ptr = this->base + ((ptr[0] & 0x3f) << 8 | ptr[1]);
    }
    assert(ptr - base < 0x4000);
    memcpy(this->buf + this->buf_end, ptr + 1, *ptr);
    this->buf[this->buf_end + *ptr] = '.';
    this->buf_end += *ptr + 1;
    ptr += *ptr + 1;
  }
}

void DnDecompressor::decompress(std::span<byte> &src) {
  assert(src.size() > 0 && src.size() >= buf_end);
  memcpy(src.data(), buf, buf_end);
  src = src.subspan(buf_end);
  this->buf_end = 0;
}

XSL_NET_DNS_NE
