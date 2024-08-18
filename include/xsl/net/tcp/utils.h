/**
 * @file utils.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief tcp utilities
 * @version 0.1
 * @date 2024-08-18
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_TCP_UTILS
#  define XSL_NET_TCP_UTILS
#  include "xsl/feature.h"
#  include "xsl/net/tcp/def.h"
#  include "xsl/sys.h"

#  include <expected>
XSL_TCP_NB
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
Task<std::expected<sys::tcp::Socket<LowerLayer>, std::error_condition>> dial(const char *host,
                                                                             const char *port,
                                                                             Poller &poller) {
  auto res
      = sys::net::Resolver{}.resolve<feature::Tcp<LowerLayer>>(host, port, sys::net::CLIENT_FLAGS);
  if (!res) {
    co_return std::unexpected{res.error()};
  }
  auto conn_res = co_await sys::tcp::connect(*res, poller);
  if (!conn_res) {
    co_return std::unexpected{conn_res.error()};
  }
  co_return std::move(*conn_res);
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
std::expected<sys::tcp::Socket<LowerLayer>, std::error_condition> serv(const char *host,
                                                                       const char *port) {
  auto addr
      = sys::net::Resolver{}.resolve<feature::Tcp<LowerLayer>>(host, port, sys::net::SERVER_FLAGS);
  if (!addr) {
    return std::unexpected(addr.error());
  }
  auto bind_res = sys::tcp::bind(*addr);
  if (!bind_res) {
    return std::unexpected(bind_res.error());
  }
  auto lres = sys::tcp::listen(*bind_res);
  if (!lres) {
    return std::unexpected(lres.error());
  }
  return sys::tcp::Socket<LowerLayer>{std::move(*bind_res)};
}
XSL_TCP_NE
#endif
