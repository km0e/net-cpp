#pragma once
#ifndef XSL_NET_H
#  define XSL_NET_H
#  include "xsl/net/def.h"
#  include "xsl/net/http.h"
#  include "xsl/net/transport.h"
#  include "xsl/net/transport/tcp.h"
#  include "xsl/net/transport/tcp/server.h"
XSL_NB
using xsl::net::create_static_handler;
using xsl::net::HTTP_METHOD_STRINGS;
using xsl::net::HttpHandlerGenerator;
using xsl::net::HttpMethod;
using xsl::net::HttpRequestView;
using xsl::net::HttpResponse;
using xsl::net::HttpResponsePart;
using xsl::net::HttpRouteContext;
using xsl::net::HttpRouteHandleResult;
using xsl::net::HttpRouter;
using xsl::net::HttpRouteResult;
// using net::HttpServer;
using xsl::net::HttpVersion;
using xsl::net::RouteErrorKind;
using xsl::net::TcpConnManager;
using xsl::net::TcpConnManagerConfig;
using xsl::net::TcpHandleHint;
using xsl::net::TcpHandler;
using xsl::net::TcpHandlerGeneratorLike;
using xsl::net::TcpHandleState;
using xsl::net::TcpRecvError;
using xsl::net::TcpRecvString;
using xsl::net::TcpRecvTasks;
using xsl::net::TcpSendError;
using xsl::net::TcpSendString;
using xsl::net::TcpSendTasks;
using xsl::net::TcpSendTasksProxy;
using xsl::net::to_string_view;

namespace tcp {
  using Server = xsl::net::transport::tcp::TcpServer;
  template <class... Flags>
  std::expected<sys::net::Socket, std::error_condition> serv(const char *host, const char *port) {
    return xsl::net::transport::tcp::tcp_serv<Flags...>(host, port);
  };
}  // namespace tcp
XSL_NE
#endif
