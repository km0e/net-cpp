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
#ifndef XSL_CORO_NET_H
#  define XSL_CORO_NET_H
#  include "xsl/asio/dns/resolver.h"
#  include "xsl/asio/gai.h"
#  include "xsl/asio/http/conn.h"
#  include "xsl/asio/http/parse.h"
#  include "xsl/asio/http/server.h"
#  include "xsl/asio/http/service.h"
#  include "xsl/asio/pipe.h"
#  include "xsl/asio/socket.h"
#  include "xsl/asio/tcp/server.h"
#  include "xsl/sys.h"

namespace xsl::asio {
  using _asio::AsyncSocket;
  using _asio::AsyncSocketCompose;
  using _asio::gai_async_connect;
  using _asio::splice_bidirectional;
  using sys::net::gai_connect;
  using sys::net::getaddrinfo;
  using sys::net::SockAddr;
  using sys::net::SockAddrCompose;
  using sys::net::Socket;
  using sys::net::SocketCompose;
  // using xsl::_net::io::splice;
  namespace tcp {
    using _asio::tcp::make_server;
    using _asio::tcp::Server;
  }  // namespace tcp

  namespace udp {}  // namespace udp
  using _asio::Resolver;
  // using xsl::_net::dns::Server;
  using _asio::ResolverImpl;

  using _asio::http::Connection;
  using _asio::http::HandleContext;
  using _asio::http::HandleResult;
  using _asio::http::Method;
  using _asio::http::ParseData;
  using _asio::http::Parser;
  using _asio::http::ParseUnit;
  using _asio::http::Request;
  using _asio::http::RequestView;
  using _asio::http::Response;
  using _asio::http::ResponsePart;
  using _asio::http::RouteContext;
  using _asio::http::Router;
  using _asio::http::StaticFileConfig;
  using _asio::http::Status;
  using _asio::http::to_string_view;

  namespace http1 {
    using _asio::http::Connection;
    using _asio::http::make_service;
    using _asio::http::Server;
    using _asio::http::Service;
  }  // namespace http1
}  // namespace xsl::asio
#endif
