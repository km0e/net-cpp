#pragma once
#ifndef XSL_SYS
#  define XSL_SYS
#  include "xsl/def.h"
#  include "xsl/sys/net.h"
#  include "xsl/sys/net/accept.h"
#  include "xsl/sys/net/resolve.h"
#  include "xsl/sys/net/tcp.h"
XSL_NB
namespace net {
  using sys::net::accept;
  using sys::net::AcceptResult;
  using sys::net::CLIENT_FLAGS;
  using sys::net::Resolver;
  using sys::net::SERVER_FLAGS;
}  // namespace net
namespace sys::tcp {
  using sys::net::bind;
  using sys::net::connect;
  using sys::net::listen;
}  // namespace sys::tcp
XSL_NE
#endif
