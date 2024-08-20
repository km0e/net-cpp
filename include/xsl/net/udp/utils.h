/**
 * @file utils.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
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
/**
 * @brief Dial a tcp connection to a host and port
 *
 * @tparam LowerLayer the lower layer to use, such as feature::Ip<Version>(Version = 4 or 6)
 * @param host the host to dial
 * @param port the port to dial
 * @param poller the poller to use
 * @return Task<std::expected<sys::tcp::Socket<LowerLayer>, std::error_condition>>
 */
template <class LowerLayer>
std::expected<sys::udp::Socket<LowerLayer>, std::error_condition> dial(const char *host,
                                                                       const char *port) {
  auto res
      = sys::net::Resolver{}.resolve<feature::Udp<LowerLayer>>(host, port, sys::net::CLIENT_FLAGS);
  if (!res) {
    return std::unexpected{res.error()};
  }
  auto conn_res = sys::net::connect(*res);
  if (!conn_res) {
    return std::unexpected{conn_res.error()};
  }
  return std::move(*conn_res);
}
/**
 * @brief Create a tcp accept socket
 *
 * @tparam LowerLayer the lower layer to use, such as feature::Ip<Version>(Version = 4 or 6)
 * @param host the host to listen on
 * @param port the port to listen on
 * @return std::expected<sys::tcp::Socket<LowerLayer>, std::error_condition>
 */
template <class LowerLayer>
std::expected<sys::udp::Socket<LowerLayer>, std::error_condition> serv(const char *host,
                                                                       const char *port) {
  auto addr
      = sys::net::Resolver{}.resolve<feature::Udp<LowerLayer>>(host, port, sys::net::SERVER_FLAGS);
  if (!addr) {
    return std::unexpected(addr.error());
  }
  auto bind_res = sys::net::bind(*addr);
  if (!bind_res) {
    return std::unexpected{bind_res.error()};
  }
  return std::move(*bind_res);
}
XSL_UDP_NE
#endif
