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
using net::HTTP_METHOD_STRINGS;
using net::http_version_cast;
using net::HttpHandlerGenerator;
using net::HttpMethod;
using net::HttpParseError;
using net::HttpParser;
using net::HttpParseResult;
using net::HttpRequestView;
using net::HttpResponsePart;
using net::HttpRouter;
using net::HttpServer;
using net::HttpServerConfig;
using net::IOM_EVENTS;
using net::Poller;
using net::PollHandler;
using net::RouteErrorKind;
using net::sub_shared;
using net::sub_unique;
using net::TcpHandleConfig;
using net::TcpHandleHint;
using net::TcpHandlerGenerator;
using net::TcpHandleState;
using net::TcpRecvString;
using net::TcpRecvTasks;
using net::TcpSendString;
using net::TcpSendTasks;
using net::TcpServer;
using net::TcpServerConfig;
XSL_NAMESPACE_END
#endif
