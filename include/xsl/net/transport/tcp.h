#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_H_
#  define _XSL_NET_TRANSPORT_TCP_H_
#  include "xsl/net/transport/def.h"
#  include "xsl/net/transport/tcp/component.h"
#  include "xsl/net/transport/tcp/conn.h"
#  include "xsl/net/transport/tcp/server.h"
#  include "xsl/net/transport/tcp/utils.h"
TRANSPORT_NAMESPACE_BEGIN
using TcpHandleHint = tcp::HandleHint;
using tcp::TcpHandlerGeneratorLike;
using tcp::TcpHandlerLike;
using TcpHandleState = tcp::HandleState;
using tcp::TcpRecvString;
using TcpSendTaskNode = tcp::SendTaskNode;
using TcpRecvTaskNode = tcp::RecvTaskNode;
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
using tcp::connect;
using tcp::recv;
using tcp::send;
using tcp::SockAddrV4;
using tcp::SockAddrV4View;
using tcp::TcpConnManager;
using tcp::TcpConnManagerConfig;
using tcp::TcpHandler;
using tcp::TcpServer;
using tcp::to_string_view;
TRANSPORT_NAMESPACE_END
#endif
