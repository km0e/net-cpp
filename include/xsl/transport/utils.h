#pragma once
#ifndef _XSL_NET_TRANSPORT_UTILS_H_
#  define _XSL_NET_TRANSPORT_UTILS_H_
#  include <xsl/transport/transport.h>
#  include <xsl/utils/wheel/wheel.h>
TRANSPORT_NAMESPACE_BEGIN

int create_tcp_client(const char *ip, const char *port);
const int MAX_CONNECTIONS = 10;
class TcpConfig {
public:
  int max_connections = MAX_CONNECTIONS;
};
int create_tcp_server(const char *host, int port, TcpConfig config = {});
int read(int fd, wheel::string &data);
TRANSPORT_NAMESPACE_END
#endif  // _XSL_NET_TRANSPORT_UTILS_H_
