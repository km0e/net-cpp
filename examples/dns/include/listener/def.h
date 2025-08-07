#pragma once
#ifndef XSL_APP_DNS_LISTENER_DEF_H
#  define XSL_APP_DNS_LISTENER_DEF_H
#  include <dns.h>
#  include <xsl/asio.h>
#  include <xsl/net.h>

using namespace xsl;

struct Listener {
  virtual ~Listener() = default;
  virtual Task<void> run(DnsCore &core) = 0;
};

#endif
