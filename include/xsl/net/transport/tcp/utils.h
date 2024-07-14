#pragma once
#ifndef XSL_NET_TRANSPORT_UTILS
#  define XSL_NET_TRANSPORT_UTILS
#  include "xsl/coro/task.h"
#  include "xsl/net/transport/resolve.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/sync/poller.h"
#  include "xsl/sys.h"

#  include <netdb.h>
#  include <sys/socket.h>

#  include <expected>
#  include <generator>
#  include <memory>
#  include <system_error>

TCP_NB

using ConnectResult = std::expected<Socket, std::errc>;

coro::Task<ConnectResult> connect(const AddrInfo &ai, std::shared_ptr<sync::Poller> poller);

using BindResult = std::expected<Socket, std::errc>;

BindResult bind(const AddrInfo &ai);

std::expected<void, std::error_condition> listen(Socket &skt, int max_connections = 10);

bool set_keep_alive(int fd, bool keep_alive = true);

// std::string_view to_string_view(SendError err);

// std::string_view to_string_view(RecvError err);
// using RecvResult = std::expected<std::string, RecvError>;
// SendResult send(int fd, std::string_view data);
// RecvResult recv(int fd);
TCP_NE
#endif  // XSL_NET_TRANSPORT_UTILS
