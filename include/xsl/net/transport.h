#pragma once
#ifndef _XSL_NET_TRANSPORT_H_
#  define _XSL_NET_TRANSPORT_H_
#  include "xsl/net/def.h"
#  include "xsl/net/transport/tcp.h"
NET_NAMESPACE_BEGIN
using transport::detail::create_tcp_client;
using transport::detail::create_tcp_server;
using transport::detail::TcpHandleConfig;
using transport::detail::TcpHandleHint;
using transport::detail::TcpHandler;
using transport::detail::TcpHandleState;
using transport::detail::TcpRecvString;
using transport::detail::TcpRecvTasks;
using transport::detail::TcpSendFile;
using transport::detail::TcpSendString;
using transport::detail::TcpSendTasks;
using transport::detail::TcpServer;
using transport::detail::TcpServerConfig;
NET_NAMESPACE_END
#endif  // _XSL_NET_TRANSPORT_H_
