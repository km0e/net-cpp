/**
 * @file net.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Network utilities
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_H
#  define XSL_NET_H
#  include "xsl/net/io/splice.h"
#  include "xsl/net/tcp/server.h"
XSL_NB
namespace net {
  using sys::net::gai_async_connect;
  using sys::net::gai_bind;
  using sys::net::gai_connect;
  using xsl::_net::io::splice;
}  // namespace net
namespace tcp {
  using xsl::_net::tcp::make_server;
  using xsl::_net::tcp::Server;
}  // namespace tcp

namespace udp {}  // namespace udp

XSL_NE
#endif
