/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS Resolver definition
 * @version 0.1.0
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_APP_DNS_RESOLVER_DEF_H
#  define XSL_APP_DNS_RESOLVER_DEF_H
#  include <xsl/asio.h>
#  include <xsl/net.h>

using namespace xsl;

using DnsBuf = FixedBuffer<dns::MAX_SIZE_DNS_UDP>;
using DnsSig = SPSCSignal2<1>;

struct Resolver {
  virtual ~Resolver() = default;
  virtual Task<std::expected<std::pair<std::vector<dns::RRView>, DnsBuf>, errc>> query(
      std::string_view dn, dns::Question &q)
      = 0;
};

#endif
