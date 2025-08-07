/**
 * @file sys.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief System utilities
 * @version 0.1.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS
#  define XSL_SYS
#  include <xsl/def.h>
#  include <xsl/sys/io.h>
#  include <xsl/sys/net/def.h>
#  include <xsl/sys/net/gai.h>
#  include <xsl/sys/net/sockaddr.h>
#  include <xsl/sys/net/socket.h>
#  include <xsl/sys/net/utils.h>
#  include <xsl/sys/raw.h>

XSL_NB
namespace sys::net {
  using _sys::net::AddrInfos;
  using _sys::net::ConnectionBasedSocketTraits;
  using _sys::net::ConnectionUtils;
  using _sys::net::gai_bind;
  using _sys::net::gai_connect;
  using _sys::net::getaddrinfo;
  using _sys::net::make_sockaddr;
  using _sys::net::SockAddr;
  using _sys::net::SockAddrCompose;
  using _sys::net::Socket;
  using _sys::net::SocketAttribute;
  using _sys::net::SocketCompose;
  using _sys::net::SocketTraits;
  using _sys::net::SocketTraitsCompatible;
  namespace dns {
    using _sys::net::CLIENT_FLAGS;
    using _sys::net::SERVER_FLAGS;
  }  // namespace dns
}  // namespace sys::net
namespace sys {
  using _sys::current_ec;
  using _sys::filter_interrupt;
  using _sys::read;
  using _sys::write;
}  // namespace sys
namespace sys::tcp {

  template <class LowerLayer>
  using Socket = net::Socket<Tcp<LowerLayer>>;

}  // namespace sys::tcp
namespace sys::udp {

  template <class LowerLayer>
  using Socket = net::Socket<Udp<LowerLayer>>;

}  // namespace sys::udp
using _sys::RawHandle;
using _sys::RawOwner;
XSL_NE
#endif
