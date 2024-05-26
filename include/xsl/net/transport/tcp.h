#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_H_
#  define _XSL_NET_TRANSPORT_TCP_H_
#  include "xsl/net/transport/def.h"
#  include "xsl/net/transport/tcp/client.h"
#  include "xsl/net/transport/tcp/conn.h"
#  include "xsl/net/transport/tcp/context.h"
#  include "xsl/net/transport/tcp/helper.h"
#  include "xsl/net/transport/tcp/server.h"
#  include "xsl/net/transport/tcp/utils.h"
TRANSPORT_NAMESPACE_BEGIN
using TcpHandleConfig = tcp::HandleConfig;
using TcpHandleHint = tcp::HandleHint;
using tcp::TcpHandler;
using tcp::TcpHandlerGenerator;
using TcpHandleState = tcp::HandleState;
using TcpRecvString = tcp::RecvString;
using TcpSendString = tcp::SendString;
using TcpSendTaskNode = tcp::SendTaskNode;
using TcpRecvTaskNode = tcp::RecvTaskNode;
using tcp::TcpClientConfig;
using tcp::TcpServerConfig;
using TcpRecvTasks = tcp::RecvTasks;
using TcpSendFile = tcp::SendFile;
using TcpSendString = tcp::SendString;
using TcpSendResult = tcp::SendResult;
using TcpSendContext = tcp::SendContext;
using TcpRecvResult = tcp::RecvResult;
using TcpRecvContext = tcp::RecvContext;
using TcpSendTasks = tcp::SendTasks;
using tcp::create_tcp_client;
using tcp::create_tcp_server;
using tcp::TcpClient;
using tcp::TcpServer;
using tcp::TcpServerSockConfig;
using tcp::TcpClientSockConfig;
TRANSPORT_NAMESPACE_END
#endif
