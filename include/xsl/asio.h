/**
 * @file net.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Network utilities
 * @version 0.2.3
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_H
#  define XSL_ASIO_H
#  include <xsl/asio/def.h>
#  include <xsl/asio/http.h>
#  include <xsl/asio/http/request.h>
#  include <xsl/asio/http/server.h>
#  include <xsl/asio/http/service.h>
#  include <xsl/asio/net.h>
#  include <xsl/asio/pipe.h>
#  include <xsl/error.h>
#  include <xsl/feature.h>
#  include <xsl/sys.h>

namespace xsl::asio {
  using sys::net::gai_connect;
  using sys::net::getaddrinfo;
  using sys::net::SockAddr;
  using sys::net::SockAddrCompose;
  using sys::net::Socket;
  using sys::net::SocketCompose;
  //

  namespace udp {}  // namespace udp

}  // namespace xsl::asio
#endif
