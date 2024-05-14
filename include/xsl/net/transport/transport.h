#pragma once
#ifndef _XSL_NET_TRANSPORT_H_
#  define _XSL_NET_TRANSPORT_H_
#  include "xsl/net/transport/tcp/tcp.h"
namespace xsl::net::transport {
  using tcp::create_tcp_client;
  using tcp::create_tcp_server;
  using tcp::TcpHandleConfig;
  using tcp::TcpHandleHint;
  using tcp::TcpHandler;
  using tcp::TcpHandleState;
  using tcp::TcpRecvString;
  using tcp::TcpRecvTasks;
  using tcp::TcpSendFile;
  using tcp::TcpSendString;
  using tcp::TcpSendTasks;
  using tcp::TcpServer;
  using tcp::TcpServerConfig;
}  // namespace xsl::net::transport

#endif  // _XSL_NET_TRANSPORT_H_
