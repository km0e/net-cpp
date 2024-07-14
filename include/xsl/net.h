#pragma once
#ifndef XSL_NET_H
#  define XSL_NET_H
#  include "xsl/def.h"
#  include "xsl/net/http.h"
#  include "xsl/net/transport.h"
XSL_NB
using net::create_static_handler;
using net::HTTP_METHOD_STRINGS;
using net::HttpHandlerGenerator;
using net::HttpMethod;
using net::HttpRequestView;
using net::HttpResponse;
using net::HttpResponsePart;
using net::HttpRouteContext;
using net::HttpRouteHandleResult;
using net::HttpRouter;
using net::HttpRouteResult;
using net::HttpServer;
using net::HttpVersion;
using net::RouteErrorKind;
using net::TcpConnManager;
using net::TcpConnManagerConfig;
using net::TcpHandleHint;
using net::TcpHandler;
using net::TcpHandlerGeneratorLike;
using net::TcpHandleState;
using net::TcpRecvError;
using net::TcpRecvString;
using net::TcpRecvTasks;
using net::TcpSendError;
using net::TcpSendString;
using net::TcpSendTasks;
using net::TcpSendTasksProxy;
using net::TcpServer;
using net::to_string_view;
XSL_NE
#endif
