#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_H_
#  define _XSL_NET_TRANSPORT_TCP_H_
#  include "xsl/transport/tcp/conn.h"
#  include "xsl/transport/tcp/context.h"
#  include "xsl/transport/tcp/helper.h"
#  include "xsl/transport/tcp/server.h"
namespace xsl::transport::tcp {
  using detail::HandleConfig;
  using detail::HandleHint;
  using detail::Handler;
  using detail::HandleState;
  using detail::RecvString;
  using detail::RecvTasks;
  using detail::SendString;
  using detail::SendTasks;
  using detail::SendFile;
  using detail::TcpServer;
}  // namespace xsl::transport::tcp
#endif
