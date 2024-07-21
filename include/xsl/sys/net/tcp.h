#pragma once
#ifndef XSL_SYS_NET_TCP
#  define XSL_SYS_NET_TCP
#  include "xsl/coro/task.h"
#  include "xsl/sync/poller.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/endpoint.h"
#  include "xsl/sys/net/resolve.h"
#  include "xsl/sys/net/socket.h"

#  include <expected>
#  include <memory>
SYS_NET_NB

using AcceptResult = std::expected<std::tuple<Socket, SockAddr>, std::errc>;

AcceptResult accept(int fd);

using ConnectResult = std::expected<Socket, std::errc>;

decltype(auto) connect(const Endpoint &ep, std::shared_ptr<sync::Poller> poller);
coro::Task<ConnectResult> connect(const EndpointSet &eps, std::shared_ptr<sync::Poller> poller);

using BindResult = std::expected<Socket, std::errc>;

BindResult bind(const Endpoint &ep);
BindResult bind(const EndpointSet &eps);

std::expected<void, std::errc> listen(Socket &skt, int max_connections = 10);

SYS_NET_NE
#endif
