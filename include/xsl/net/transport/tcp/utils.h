#pragma once
#ifndef XSL_NET_TRANSPORT_UTILS
#  define XSL_NET_TRANSPORT_UTILS
#  include "xsl/net/transport/tcp/def.h"

#  include <netdb.h>
#  include <sys/socket.h>

#  include <expected>
#  include <generator>

TCP_NB

bool set_keep_alive(int fd, bool keep_alive = true);

// std::string_view to_string_view(SendError err);

// std::string_view to_string_view(RecvError err);
// using RecvResult = std::expected<std::string, RecvError>;
// SendResult send(int fd, std::string_view data);
// RecvResult recv(int fd);
TCP_NE
#endif  // XSL_NET_TRANSPORT_UTILS
