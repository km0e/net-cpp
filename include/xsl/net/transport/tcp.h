#pragma once

#ifndef XSL_NET_TRANSPORT_TCP
#  define XSL_NET_TRANSPORT_TCP
#  include "xsl/net/transport/def.h"
#  include "xsl/net/transport/tcp/accept.h"
#  include "xsl/net/transport/tcp/component.h"
#  include "xsl/net/transport/tcp/conn.h"
#  include "xsl/net/transport/tcp/stream.h"
#  include "xsl/net/transport/tcp/utils.h"

#  include <expected>

TRANSPORT_NB
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
using TcpSendTasksProxy = tcp::SendTasksProxy;
using TcpSendContext = tcp::SendContext;
using TcpRecvResult = tcp::RecvResult;
using TcpRecvError = tcp::RecvError;
using TcpSendError = tcp::SendError;
using TcpRecvContext = tcp::RecvContext;
using TcpSendTasks = tcp::SendTasks;
using tcp::Acceptor;
using tcp::bind;
using tcp::connect;
using tcp::listen;
using tcp::TcpConnManager;
using tcp::TcpConnManagerConfig;
using tcp::TcpHandler;
// using tcp::TcpServer;
using tcp::TcpStream;
TRANSPORT_NE
#endif
