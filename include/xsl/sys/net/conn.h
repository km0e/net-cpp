/**
 * @file conn.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Connection based functions
 * @version 0.1
 * @date 2024-08-19
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_CONN
#  define XSL_SYS_NET_CONN
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/socket.h"

#  include <expected>
#  include <system_error>
XSL_SYS_NET_NB

template <CSocket S>
constexpr std::expected<Socket<typename S::socket_traits_type>, std::errc> accept(
    S &socket, SockAddr<typename S::socket_traits_type> *addr) {
  auto [sockaddr, addrlen]
      = addr == nullptr ? SockAddr<typename S::socket_traits_type>::null() : addr->raw();
  int tmp_fd = ::accept4(socket.raw(), sockaddr, addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (tmp_fd < 0) {
    return std::unexpected{std::errc(errno)};
  }
  LOG5("accept socket {}", tmp_fd);
  // char ip[NI_MAXHOST], port[NI_MAXSERV];
  // if (getnameinfo(&addr, addrlen, ip, NI_MAXHOST, port, NI_MAXSERV, NI_NUMERICHOST |
  // NI_NUMERICSERV)
  //     != 0) {
  //   return std::unexpected{std::errc(errno)};
  // }
  return Socket<typename S::socket_traits_type>(tmp_fd);
}

template <CSocket S>
constexpr std::expected<void, std::errc> listen(S &skt, int max_connections = 10) {
  if (::listen(skt.raw(), max_connections) == -1) {
    return std::unexpected{std::errc{errno}};
  }
  return {};
}

XSL_SYS_NET_NE
#endif
