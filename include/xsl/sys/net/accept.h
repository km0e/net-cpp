#pragma once
#ifndef XSL_SYS_NET_ACCEPT
#  define XSL_SYS_NET_ACCEPT
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/socket.h"

#  include <expected>
SYS_NET_NB

using AcceptResult = std::expected<Socket, std::errc>;

AcceptResult accept(Socket &socket, SockAddr *addr);

SYS_NET_NE
#endif
