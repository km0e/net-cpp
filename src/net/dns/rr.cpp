/**
 * @file rr.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Resource Record
 * @version 0.1
 * @date 2024-09-09
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/net/dns/proto/rr.h"
#include "xsl/net/dns/utils.h"
#include "xsl/ser.h"
#include "xsl/wheel.h"

#include <netinet/in.h>

#include <cstddef>
#include <expected>
#include <memory>
#include <utility>
XSL_NET_DNS_NB
std::expected<std::pair<std::string, RR>, std::errc> deserialized(std::span<const byte> &src,
                                                                 DnDecompressor &decompressor) {
  auto ec = decompressor.prepare(src);
  if (ec != std::errc{}) {
    return std::unexpected{ec};
  }
  uint16_t u16;
  xsl::deserialize(src.data() + 8, u16);
  std::size_t rest_length = 2 + 2 + 4 + 2 + ntohs(u16);
  if (rest_length > src.size()) {  // not enough data
    return std::unexpected{std::errc::result_out_of_range};
  }
  auto name = std::string(decompressor.needed(), '\0');
  auto name_span = xsl::as_writable_bytes(std::span{name});
  decompressor.decompress(name_span);
  auto ptr = std::make_unique<byte[]>(rest_length);
  memcpy(ptr.get(), src.data(), rest_length);
  src = src.subspan(rest_length);
  return std::pair{name, RR{std::move(ptr)}};
}
XSL_NET_DNS_NE
