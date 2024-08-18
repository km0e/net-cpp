#pragma once
#ifndef XSL_SYS_NET_ACCEPT
#  define XSL_SYS_NET_ACCEPT
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/socket.h"

#  include <expected>
XSL_SYS_NET_NB

template <CSocket S>
std::expected<Socket<typename S::socket_traits_type>, std::errc> accept(S &socket, SockAddr *addr) {
  auto [sockaddr, addrlen] = addr == nullptr ? SockAddr::null() : addr->raw();
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
XSL_SYS_NET_NE
#endif
