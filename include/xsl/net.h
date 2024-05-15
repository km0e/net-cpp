#pragma once
#ifndef _XSL_NET_H
#  define _XSL_NET_H
#  include "xsl/def.h"
#  include "xsl/net/http.h"
#  include "xsl/net/sync.h"
#  include "xsl/net/transport.h"
XSL_NAMESPACE_BEGIN
using net::create_static_handler;
using net::create_tcp_client;
using net::DefaultPoller;
using net::HttpHandlerGenerator;
using net::HttpMethod;
using net::HttpParser;
using net::HttpRouter;
using net::HttpServer;
using net::HttpServerConfig;
using net::IOM_EVENTS;
using net::RouteErrorKind;
using net::TcpServer;
using net::TcpServerConfig;
XSL_NAMESPACE_END
#endif
