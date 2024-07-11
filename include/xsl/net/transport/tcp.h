#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_H_
#  define _XSL_NET_TRANSPORT_TCP_H_
#  include "xsl/net/transport/def.h"
#  include "xsl/net/transport/resolve.h"
#  include "xsl/net/transport/tcp/accept.h"
#  include "xsl/net/transport/tcp/component.h"
#  include "xsl/net/transport/tcp/conn.h"
#  include "xsl/net/transport/tcp/server.h"
#  include "xsl/net/transport/tcp/stream.h"
#  include "xsl/net/transport/tcp/utils.h"

#  include <expected>
#  include <system_error>
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
using tcp::TcpServer;
using tcp::TcpStream;

template <class... Flags>
std::expected<Socket, std::error_condition> serv(const char *host, const char *port) {
  auto addr = Resolver{}.resolve<Flags...>(host, port, SERVER_FLAGS);
  if (!addr) {
    return std::unexpected(addr.error());
  }
  auto bres = bind(addr.value());
  if (!bres) {
    return std::unexpected(bres.error());
  }
  auto lres = listen(bres.value());
  if (!lres) {
    return std::unexpected(lres.error());
  }
  return Socket{std::move(bres.value())};
}

TRANSPORT_NAMESPACE_END
#endif
