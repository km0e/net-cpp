#pragma once
#ifndef XSL_SYS
#  define XSL_SYS
#  include "xsl/def.h"
#  include "xsl/sys/io.h"
#  include "xsl/sys/net/conn.h"
#  include "xsl/sys/net/connect.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/io.h"
#  include "xsl/sys/net/resolve.h"
#  include "xsl/sys/net/socket.h"
#  include "xsl/sys/net/utils.h"
#  include "xsl/sys/raw.h"
#  include "xsl/sys/sync.h"
XSL_NB
namespace sys::net {
  using _sys::net::AsyncSocket;
  using _sys::net::bind;
  using _sys::net::CLIENT_FLAGS;
  using _sys::net::connect;
  using _sys::net::imm_recv;
  using _sys::net::imm_recvfrom;
  using _sys::net::imm_send;
  using _sys::net::imm_sendto;
  using _sys::net::ResolveFlag;
  using _sys::net::Resolver;
  using _sys::net::SERVER_FLAGS;
  using _sys::net::SockAddr;
  using _sys::net::Socket;
}  // namespace sys::net
namespace sys {
  using _sys::AsyncRawDevice;
  using _sys::DefaultDeviceTraits;
  using _sys::imm_sendfile;
  using _sys::SendfileHint;
}  // namespace sys
namespace sys::tcp {

  template <class LowerLayer>
  using Socket = net::Socket<feature::Tcp<LowerLayer>>;

  template <class LowerLayer>
  using AsyncSocket = net::AsyncSocket<feature::Tcp<LowerLayer>>;

  using _sys::net::accept;
  using _sys::net::listen;
}  // namespace sys::tcp
namespace sys::udp {

  template <class LowerLayer>
  using Socket = net::Socket<feature::Udp<LowerLayer>>;

  template <class LowerLayer>
  using AsyncSocket = net::AsyncSocket<feature::Udp<LowerLayer>>;

}  // namespace sys::udp
using _sys::IOM_EVENTS;
using _sys::Poller;
XSL_NE
#endif
