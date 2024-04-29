#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_SERVER_H_
#define _XSL_NET_TRANSPORT_TCP_SERVER_H_
#include <transport.h>
#include <poller.h>
class TcpServer {
public:
  TcpServer(wheel::string host, int port);
  bool valid();
  void set_handler(Handler handler);
  void poller_register(wheel::shared_ptr<Poller> poller);
private:
  int server_fd;
  Handler handler;
};
#endif
