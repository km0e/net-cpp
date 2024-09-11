/**
 * @file sys.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief System utilities
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS
#  define XSL_SYS
#  include "xsl/def.h"
#  include "xsl/sys/io.h"
#  include "xsl/sys/net/conn.h"
#  include "xsl/sys/net/connect.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/dns.h"
#  include "xsl/sys/net/io.h"
#  include "xsl/sys/net/sockaddr.h"
#  include "xsl/sys/net/socket.h"
#  include "xsl/sys/net/utils.h"
#  include "xsl/sys/sync.h"
XSL_NB
namespace sys::net {
  using _sys::net::AddrInfos;
  using _sys::net::AsyncSocket;
  using _sys::net::bind;
  using _sys::net::connect;
  using _sys::net::getaddrinfo;
  using _sys::net::recv;
  using _sys::net::recvfrom;
  using _sys::net::send;
  using _sys::net::sendto;
  using _sys::net::SockAddr;
  using _sys::net::SockAddrCompose;
  using _sys::net::Socket;
  using _sys::net::SocketTraits;
  namespace dns {
    using _sys::net::CLIENT_FLAGS;
    using _sys::net::SERVER_FLAGS;
  }  // namespace dns
}  // namespace sys::net
namespace sys {
  using _sys::RawAsyncDevice;
  using _sys::RawDevice;
  using _sys::read;
  using _sys::write;
}  // namespace sys
namespace sys::tcp {

  template <class LowerLayer>
  using Socket = net::Socket<Tcp<LowerLayer>>;

  template <class LowerLayer>
  using AsyncSocket = net::AsyncSocket<Tcp<LowerLayer>>;

  using _sys::net::accept;
  using _sys::net::listen;
}  // namespace sys::tcp
namespace sys::udp {

  template <class LowerLayer>
  using Socket = net::Socket<Udp<LowerLayer>>;

  template <class LowerLayer>
  using AsyncSocket = net::AsyncSocket<Udp<LowerLayer>>;

}  // namespace sys::udp
using _sys::IOM_EVENTS;
using _sys::Poller;
XSL_NE
#endif
