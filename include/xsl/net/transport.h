#pragma once
#ifndef _XSL_NET_TRANSPORT_H_
#  define _XSL_NET_TRANSPORT_H_
#  include "xsl/net/def.h"
#  include "xsl/net/transport/tcp.h"
NET_NAMESPACE_BEGIN
using transport::recv;
using transport::send;
using transport::SockAddrV4;
using transport::SockAddrV4View;
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
using transport::TcpServer;
NET_NAMESPACE_END
#endif  // _XSL_NET_TRANSPORT_H_
