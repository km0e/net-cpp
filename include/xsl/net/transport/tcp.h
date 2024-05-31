#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_H_
#  define _XSL_NET_TRANSPORT_TCP_H_
#  include "xsl/net/transport/def.h"
#  include "xsl/net/transport/tcp/client.h"
#  include "xsl/net/transport/tcp/component.h"
#  include "xsl/net/transport/tcp/conn.h"
#  include "xsl/net/transport/tcp/server.h"
#  include "xsl/net/transport/tcp/utils.h"
TRANSPORT_NAMESPACE_BEGIN
using TcpHandleHint = tcp::HandleHint;
using tcp::TcpHandlerLike;
using tcp::TcpHandlerGeneratorLike;
using TcpHandleState = tcp::HandleState;
using tcp::TcpRecvString;
using TcpSendTaskNode = tcp::SendTaskNode;
using TcpRecvTaskNode = tcp::RecvTaskNode;
using tcp::TcpClientConfig;
using tcp::TcpServerConfig;
using TcpRecvTasks = tcp::RecvTasks;
using TcpSendFile = tcp::SendFile;
using tcp::TcpRecvString;
using tcp::TcpSendString;
using TcpSendResult = tcp::SendResult;
using TcpSendTasksProxy = tcp::SendTasksProxy;
using TcpSendContext = tcp::SendContext;
using TcpRecvResult = tcp::RecvResult;
using TcpRecvError = tcp::RecvError;
using TcpSendError = tcp::SendError;
using TcpRecvContext = tcp::RecvContext;
using TcpSendTasks = tcp::SendTasks;
using tcp::create_tcp_client;
using tcp::create_tcp_server;
using tcp::TcpClient;
using tcp::TcpClientSockConfig;
using tcp::TcpServer;
using tcp::TcpServerSockConfig;
using tcp::to_string;
TRANSPORT_NAMESPACE_END
#endif
