#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_H_
#  define _XSL_NET_TRANSPORT_TCP_H_
#  include "xsl/net/transport/tcp/conn.h"
#  include "xsl/net/transport/tcp/context.h"
#  include "xsl/net/transport/tcp/helper.h"
#  include "xsl/net/transport/tcp/server.h"
#  include "xsl/net/transport/tcp/utils.h"
namespace xsl::net::transport::tcp {
  using TcpHandleConfig = detail::HandleConfig;
  using TcpHandleHint = detail::HandleHint;
  using detail::TcpHandler;
  using TcpHandleState = detail::HandleState;
  using TcpRecvString = detail::RecvString;
  using detail::TcpServerConfig;
  using TcpRecvTasks = detail::RecvTasks;
  using TcpSendFile = detail::SendFile;
  using TcpSendString = detail::SendString;
  using TcpSendTasks = detail::SendTasks;
  using detail::create_tcp_client;
  using detail::create_tcp_server;
  using detail::TcpServer;
}  // namespace xsl::net::transport::tcp
#endif
