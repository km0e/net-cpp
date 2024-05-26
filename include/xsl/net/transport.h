#pragma once
#ifndef _XSL_NET_TRANSPORT_H_
#  define _XSL_NET_TRANSPORT_H_
#  include "xsl/net/def.h"
#  include "xsl/net/transport/tcp.h"
NET_NAMESPACE_BEGIN
using transport::create_tcp_client;
using transport::create_tcp_server;
using transport::TcpClientSockConfig;
using transport::TcpHandleConfig;
using transport::TcpHandleHint;
using transport::TcpHandler;
using transport::TcpHandlerGenerator;
using transport::TcpHandleState;
using transport::TcpRecvString;
using transport::TcpRecvTasks;
using transport::TcpSendFile;
using transport::TcpSendString;
using transport::TcpSendTasks;
using transport::TcpServer;
using transport::TcpServerConfig;
using transport::TcpServerSockConfig;
NET_NAMESPACE_END
#endif  // _XSL_NET_TRANSPORT_H_
