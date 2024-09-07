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

#  include <expected>
XSL_SYS_NET_NB

template <class S, class Decayed = std::decay_t<S>>
  requires requires { Decayed::accept; }
constexpr decltype(auto) accept(S &socket, SockAddr<typename Decayed::socket_traits_type> *addr) {
  return socket.accept(addr);
}

template <class S, class Decayed = std::decay_t<S>>
  requires requires { Decayed::listen; }
constexpr decltype(auto) listen(S &skt, int max_connections = 10) {
  return skt.listen(max_connections);
}

XSL_SYS_NET_NE
#endif
