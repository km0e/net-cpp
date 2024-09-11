/**
 * @file utils.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
 * @date 2024-08-19
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_UDP_UTILS
#  define XSL_NET_UDP_UTILS
#  include "xsl/feature.h"
#  include "xsl/net/udp/def.h"
#  include "xsl/sys.h"

#  include <expected>
XSL_UDP_NB
using namespace sys::net;
/**
 * @brief Dial a tcp connection to a host and port
 *
 * @tparam LowerLayer the lower layer to use, such as Ip<Version>(Version = 4 or 6)
 * @param host the host to dial
 * @param port the port to dial
 * @param poller the poller to use
 * @return Task<std::expected<sys::tcp::Socket<LowerLayer>, std::error_condition>>
 */
template <class LowerLayer>
constexpr std::expected<sys::udp::Socket<LowerLayer>, std::error_condition> dial(const char *host,
                                                                                 const char *port) {
  auto res = getaddrinfo<Udp<LowerLayer>>(host, port, sys::net::dns::CLIENT_FLAGS);
  if (!res) {
    return std::unexpected{res.error()};
  }
  auto conn_res = sys::net::connect(*res);
  if (!conn_res) {
    return std::unexpected{conn_res.error()};
  }
  return std::move(*conn_res);
}
/// @brief dial a connection to a socket address
template <class LowerLayer>
constexpr std::expected<sys::udp::Socket<LowerLayer>, std::error_condition> dial(
    sys::net::SockAddr<Udp<LowerLayer>> sa) {
  return sys::net::connect(sa);
}
/**
 * @brief Create a tcp accept socket
 *
 * @tparam LowerLayer the lower layer to use, such as Ip<Version>(Version = 4 or 6)
 * @param host the host to listen on
 * @param port the port to listen on
 * @return std::expected<sys::tcp::Socket<LowerLayer>, std::error_condition>
 */
template <class LowerLayer>
constexpr std::expected<sys::udp::Socket<LowerLayer>, std::error_condition> serv(const char *host,
                                                                                 const char *port) {
  auto addr = getaddrinfo<Udp<LowerLayer>>(host, port, sys::net::dns::SERVER_FLAGS);
  if (!addr) {
    return std::unexpected(addr.error());
  }
  auto bind_res = sys::net::bind(*addr);
  if (!bind_res) {
    return std::unexpected{bind_res.error()};
  }
  return std::move(*bind_res);
}
/// @brief create a server socket from a socket address
template <class LowerLayer>
constexpr std::expected<sys::udp::Socket<LowerLayer>, std::error_condition> serv(
    sys::net::SockAddr<Udp<LowerLayer>> sa) {
  return sys::net::bind(sa);
}
XSL_UDP_NE
#endif
