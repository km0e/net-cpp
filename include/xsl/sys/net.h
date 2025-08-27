/**
 * @file net.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Sys Net utilities
 * @version 0.1.0
 * @date 2025-08-29
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_SYS_NET
#  define XSL_SYS_NET
#  include <xsl/sys/def.h>
#  include <xsl/sys/net/def.h>
#  include <xsl/sys/net/socket.h>
#  include <xsl/sys/net/utils.h>
XSL_SYS_ENB
namespace net {
  namespace inet = _sys::net::inet;

  using _sys::net::AddrInfos;
  using _sys::net::ConnectionBasedSocketTraits;
  using _sys::net::ConnectionUtils;
  using _sys::net::gai_bind;
  using _sys::net::gai_connect;
  using _sys::net::GaiErrorUtil;
  using _sys::net::getaddrinfo;
  /**
   * @brief Make a socket address
   *
   * @tparam Flags list of flags, such as Ip<4>/Ip<6>, Tcp/Udp, TcpIpv4 ...
   * @param (ip:[const char*|std::string_view|const std::string&], port:[const
   * char*|std::string_view|const std::string&|uint16_t])
   * @return Expected<SockAddr<SocketTraits<Flags...>>>
   */
  using _sys::net::make_sockaddr;
  using _sys::net::make_socket_utils;
  using _sys::net::SockAddr;
  using _sys::net::SockAddrCompose;
  using _sys::net::Socket;
  using _sys::net::socket;
  using _sys::net::SocketAttribute;
  using _sys::net::SocketCompose;
  using _sys::net::SocketTraits;
  using _sys::net::SocketTraitsCompatible;
  using _sys::net::SocketUtils;
  namespace dns {
    using _sys::net::CLIENT_FLAGS;
    using _sys::net::SERVER_FLAGS;
  }  // namespace dns
}  // namespace net
XSL_SYS_ENE
#endif
