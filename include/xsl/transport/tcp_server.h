#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_SERVER_H_
#define _XSL_NET_TRANSPORT_TCP_SERVER_H_
#include <xsl/config.h>
#include <xsl/sync/poller.h>
#include <xsl/transport/transport.h>
XSL_NAMESPACE_BEGIN
TRANSPORT_NAMESPACE_BEGIN
class TcpServer {
public:
  TcpServer();
  ~TcpServer();
  bool serve(const char *ip, int port);
  bool valid();
  void set_handler(sync::Handler handler);
  void poller_register(wheel::shared_ptr<sync::Poller> poller);
private:
  int server_fd;
  sync::Handler handler;
};
TRANSPORT_NAMESPACE_END
XSL_NAMESPACE_END
#endif
