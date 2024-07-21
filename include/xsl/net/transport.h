#pragma once
#ifndef XSL_NET_TRANSPORT
#  define XSL_NET_TRANSPORT
#  include "xsl/net/def.h"
#  include "xsl/net/transport/tcp.h"
NET_NB
using transport::TcpConnManager;
using transport::TcpConnManagerConfig;
using transport::TcpHandleHint;
using transport::TcpHandler;
using transport::TcpHandlerGeneratorLike;
using transport::TcpHandlerLike;
using transport::TcpHandleState;
using transport::TcpRecvError;
using transport::TcpRecvString;
using transport::TcpRecvTasks;
using transport::TcpSendError;
using transport::TcpSendFile;
using transport::TcpSendString;
using transport::TcpSendTasks;
using transport::TcpSendTasksProxy;
using transport::TcpStream;
NET_NE
#endif  // XSL_NET_TRANSPORT
