#pragma once
#ifndef XSL_NET_H
#  define XSL_NET_H
#  include "xsl/coro/task.h"
#  include "xsl/net/def.h"
#  include "xsl/net/http/component.h"
#  include "xsl/net/http/parse.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/http/server.h"
#  include "xsl/net/io/buffer.h"
#  include "xsl/net/io/splice.h"
#  include "xsl/net/tcp.h"
// #  include "xsl/net/transport/tcp.h"
XSL_NB
namespace net {
  using xsl::_net::io::Block;
  using xsl::_net::io::Buffer;
  using xsl::_net::io::splice;
  // using net::HttpServer;
}  // namespace net
namespace tcp {
  using Server = xsl::_net::TcpServer;
  template <class... Flags>
  coro::Task<std::expected<sys::net::Socket, std::errc>> dial(const char *host, const char *port,
                                                              sync::Poller &poller) {
    return xsl::_net::tcp_dial<Flags...>(host, port, poller);
  }
  template <class... Flags>
  std::expected<sys::net::Socket, std::error_condition> serv(const char *host, const char *port) {
    return xsl::_net::tcp_serv<Flags...>(host, port);
  };
}  // namespace tcp

namespace http {
  using xsl::_net::http::create_static_handler;
  using xsl::_net::http::HttpMethod;
  using xsl::_net::http::HttpParser;
  using xsl::_net::http::HttpParseUnit;
  using xsl::_net::http::HttpResponse;
  using xsl::_net::http::HttpRouter;
  using xsl::_net::http::HttpStatus;
  using xsl::_net::http::Request;
  using xsl::_net::http::RequestView;
  using xsl::_net::http::ResponsePart;
  using xsl::_net::http::RouteContext;
  using xsl::_net::http::RouteHandleResult;
  using xsl::_net::http::RouteResult;
  using xsl::_net::io::splice;
  // using net::HttpServer;
  using xsl::_net::http::HttpServer;
  using xsl::_net::http::HttpVersion;
  using xsl::_net::http::RouteError;
  using xsl::_net::http::to_string_view;
}  // namespace http

XSL_NE
#endif
