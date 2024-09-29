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
#  include "xsl/net/dns/proto/def.h"
#  include "xsl/net/dns/proto/header.h"
#  include "xsl/net/dns/proto/question.h"
#  include "xsl/net/dns/proto/rr.h"
#  include "xsl/net/dns/resolver.h"
#  include "xsl/net/dns/utils.h"
#  include "xsl/net/http/conn.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/http/server.h"
#  include "xsl/net/http/service.h"
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
namespace dns {
  using xsl::_net::dns::Class;
  using xsl::_net::dns::dial;
  using xsl::_net::dns::DnCompressor;
  using xsl::_net::dns::DnDecompressor;
  using xsl::_net::dns::Header;
  using xsl::_net::dns::RR;
  using xsl::_net::dns::serialized;
  using xsl::_net::dns::skip_question;
  using xsl::_net::dns::Type;
  // using xsl::_net::dns::Server;
  using xsl::_net::dns::ResolverImpl;
}  // namespace dns

namespace http {
  using xsl::_net::http::Connection;
  using xsl::_net::http::HandleContext;
  using xsl::_net::http::HandleResult;
  using xsl::_net::http::Method;
  using xsl::_net::http::ParseData;
  using xsl::_net::http::Parser;
  using xsl::_net::http::ParseUnit;
  using xsl::_net::http::Request;
  using xsl::_net::http::RequestView;
  using xsl::_net::http::Response;
  using xsl::_net::http::ResponsePart;
  using xsl::_net::http::RouteContext;
  using xsl::_net::http::Router;
  using xsl::_net::http::RouteResult;
  using xsl::_net::http::StaticFileConfig;
  using xsl::_net::http::Status;
  using xsl::_net::http::to_string_view;
  using xsl::_net::http::Version;
}  // namespace http

namespace http1 {
  using xsl::_net::http::Connection;
  using xsl::_net::http::make_service;
  using xsl::_net::http::Server;
  using xsl::_net::http::Service;
}  // namespace http1

XSL_NE
#endif
