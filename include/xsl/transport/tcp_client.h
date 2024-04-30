#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_CLIENT_H_
#define _XSL_NET_TRANSPORT_TCP_CLIENT_H_
#include <xsl/config.h>
#include <xsl/transport/transport.h>
XSL_NAMESPACE_BEGIN
TRANSPORT_NAMESPACE_BEGIN
class TcpClient {
public:
  TcpClient();
  int connect(const char *host, const char *port);
private:
};
TRANSPORT_NAMESPACE_END
XSL_NAMESPACE_END
#endif