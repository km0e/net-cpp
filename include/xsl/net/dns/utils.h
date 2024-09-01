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

#  include <cstddef>
#  include <cstdint>
#  include <span>
#  include <string_view>
XSL_NET_DNS_NB

struct Status {
  int code;
  bool invalid_domain_name() const { return code == -1; }
  /**
   * @brief invalid pointer
   *
   * @return true if the pointer is invalid, that means you should drop the packet
   * @return false if the pointer is valid
   */
  bool invalid_pointer() const { return code == -2; }
  std::size_t size() const { return code; }
};

/**
 * @brief
 *
 */
class DnCompressor {
public:
  DnCompressor(const std::uint8_t *base)
      : _src(), lens(), base(base), dnptrs(), dnptrs_cnt(0), suffix_len(0), suffix_off(0) {}
  /**
   * @brief prepare the domain name for compression
   *
   * @param src domain name, maximum length is 255. If the length is greater than 255, it will be
   * truncated
   * @return std::size_t
   */
  Status prepare(std::string_view src);
  /**
   * @brief compress the domain name
   *
   * @param dst memory to store the compressed domain name
   * @note the memory size must be greater than or equal to the size returned by the prepare method
   */
  void compress(std::span<byte> dst);
  void reset();

private:
  std::string_view _src;
  std::uint8_t lens[127];

  const std::uint8_t *base;

  const std::uint8_t *dnptrs[20];
  std::size_t dnptrs_cnt;

  std::size_t suffix_len;
  std::size_t suffix_off;
};

int dn_comp(const char *src, std::uint8_t *dst, std::size_t space, std::uint8_t **dnptrs,
            std::uint8_t **lastdnptr);
XSL_NET_DNS_NE
#endif
