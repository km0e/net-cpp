#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_SERVER_H_
#define _XSL_NET_TRANSPORT_TCP_SERVER_H_
#include <poller.h>
#include <transport.h>
namespace xsl {
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
}  // namespace xsl
#endif
