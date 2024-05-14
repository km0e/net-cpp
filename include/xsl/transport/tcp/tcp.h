#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_H_
#  define _XSL_NET_TRANSPORT_TCP_H_
#  include "xsl/transport/tcp/conn.h"
#  include "xsl/transport/tcp/context.h"
#  include "xsl/transport/tcp/helper.h"
#  include "xsl/transport/tcp/server.h"
namespace xsl::transport::tcp {
  using TcpHandleConfig = detail::HandleConfig;
  using TcpHandleHint = detail::HandleHint;
  using detail::TcpHandler;
  using TcpHandleState = detail::HandleState;
  using TcpRecvString = detail::RecvString;
  using TcpRecvTasks = detail::RecvTasks;
  using TcpSendFile = detail::SendFile;
  using TcpSendString = detail::SendString;
  using TcpSendTasks = detail::SendTasks;
  using detail::TcpServer;
}  // namespace xsl::transport::tcp
#endif
