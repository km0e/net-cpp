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
using net::IOM_EVENTS;
using net::poll_add_shared;
using net::poll_add_unique;
using net::Poller;
using net::PollHandleHint;
using net::PollHandleHintTag;
using net::PollHandler;
using net::recv;
using net::RouteErrorKind;
using net::send;
using net::SockAddrV4;
using net::SockAddrV4View;
using net::TcpClientSockConfig;
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
XSL_NAMESPACE_END
#endif
