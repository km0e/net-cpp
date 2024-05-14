#pragma once
#ifndef _XSL_NET_TRANSPORT_UTILS_H_
#  define _XSL_NET_TRANSPORT_UTILS_H_
#  include "xsl/net/transport/def.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/wheel/wheel.h"
TCP_NAMESPACE_BEGIN

int create_tcp_client(const char *ip, const char *port);
class TcpConfig {
public:
  int max_connections = transport::detail::MAX_CONNECTIONS;
};
int create_tcp_server(const char *host, int port, TcpConfig config = {});
int read(int fd, wheel::string &data);
TCP_NAMESPACE_END
#endif  // _XSL_NET_TRANSPORT_UTILS_H_
