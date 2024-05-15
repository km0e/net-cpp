#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_H_
#  define _XSL_NET_TRANSPORT_TCP_H_
#  include "xsl/net/transport/def.h"
#  include "xsl/net/transport/tcp/conn.h"
#  include "xsl/net/transport/tcp/context.h"
#  include "xsl/net/transport/tcp/helper.h"
#  include "xsl/net/transport/tcp/server.h"
#  include "xsl/net/transport/tcp/utils.h"
TRANSPORT_NAMESPACE_BEGIN
using TcpHandleConfig = tcp::detail::HandleConfig;
using TcpHandleHint = tcp::detail::HandleHint;
using tcp::detail::TcpHandler;
using TcpHandleState = tcp::detail::HandleState;
using TcpRecvString = tcp::detail::RecvString;
using tcp::detail::TcpServerConfig;
using TcpRecvTasks = tcp::detail::RecvTasks;
using TcpSendFile = tcp::detail::SendFile;
using TcpSendString = tcp::detail::SendString;
using TcpSendTasks = tcp::detail::SendTasks;
using tcp::detail::create_tcp_client;
using tcp::detail::create_tcp_server;
using tcp::detail::TcpServer;
TRANSPORT_NAMESPACE_END
#endif
