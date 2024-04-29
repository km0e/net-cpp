#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_CLIENT_H_
#define _XSL_NET_TRANSPORT_TCP_CLIENT_H_
#include <transport.h>
class TcpClient {
public:
  TcpClient();
  int connect(const char *host, const char *port);
private:
};
#endif