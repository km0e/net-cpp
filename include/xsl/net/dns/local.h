#pragma once
#ifndef XSL_NET_DNS_LOCAL
#  define XSL_NET_DNS_LOCAL
#  include "xsl/net/dns/def.h"

#  include <netinet/in.h>
#  include <resolv.h>

#  include <string>
XSL_NET_DNS_NB
class Local {
public:
  Local() { res_init(); }
  ~Local() = default;
  std::string query(const std::string &name) {
    _res.nscount = 1;
  }
};
XSL_NET_DNS_NE

#endif
