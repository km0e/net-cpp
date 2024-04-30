#pragma once
#ifndef _XSL_NET_HTTP_SERVER_H_
#define _XSL_NET_HTTP_SERVER_H_
#include <xsl/config.h>
#include <xsl/sync/poller.h>
#include <xsl/transport/tcp_server.h>
#include <xsl/utils/wheel/wheel.h>
XSL_NAMESPACE_BEGIN
class HttpServer {
public:
  HttpServer();
  ~HttpServer();
  void serve(const char *ip, int port);
  void setMaxConnections(int maxConnections);
  void setMaxThreads(int maxThreads);
private:
  sync::Poller poller;
  transport::TcpServer tcpServer;
};
XSL_NAMESPACE_END
#endif