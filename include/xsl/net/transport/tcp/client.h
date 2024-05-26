#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_CLIENT_H_
#  define _XSL_NET_TRANSPORT_TCP_CLIENT_H_
#  include "xsl/net/sync/poller.h"
#  include "xsl/net/transport/tcp/conn.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/net/transport/tcp/utils.h"
#  include "xsl/wheel.h"

#  include <spdlog/spdlog.h>
#  include <unistd.h>

TCP_NAMESPACE_BEGIN
template <TcpHandler H>
class TcpClientConfig {
public:
  TcpClientConfig();
  int max_connections = MAX_CONNECTIONS;
  shared_ptr<Poller> poller;
};
template <TcpHandler H>
TcpClientConfig<H>::TcpClientConfig() : poller(nullptr) {
  // for debug
  spdlog::set_pattern("[%D-%T][%^%l%$][%t][%!] %v");
}

template <TcpHandler H>
class TcpClient {
public:
  static unique_ptr<TcpClient<H>> dial(TcpClientConfig<H> config);
  TcpClient(TcpClient&&) = delete;
  TcpClient(TcpClientConfig<H> config);
  ~TcpClient();
  bool send(const char* host, const char* port, H&& handler);

private:
  // Handler is a function that takes a shared pointer to a Poller, an int, and an IOM_EVENTS enum
  // and returns a bool The handler is called when the server receives a connection
  TcpClientConfig<H> config;
  ShareContainer<unordered_map<int, unique_ptr<TcpConn<H>>>> handlers;
};

template <TcpHandler H>
TcpClient<H>::TcpClient(TcpClientConfig<H> config) : config(config), handlers() {}

template <TcpHandler H>
TcpClient<H>::~TcpClient() {
  // TODO : close all connections
}

template <TcpHandler H>
unique_ptr<TcpClient<H>> TcpClient<H>::dial(TcpClientConfig<H> config) {
  return make_unique<TcpClient<H>>(config);
}

template <TcpHandler H>
bool TcpClient<H>::send(const char* ip, const char* port, H&& handler) {
  SPDLOG_TRACE("");
  int fd = create_tcp_client(ip, port);
  if (fd == -1) {
    SPDLOG_ERROR("Failed to create tcp client");
    return false;
  }
  auto conn = make_unique<TcpConn<H>>(fd, config.poller, move(handler));
  conn->send(fd, IOM_EVENTS::OUT);
  if (conn->valid()) {
    SPDLOG_INFO("Connected to {}:{}", ip, port);
    handlers.lock()->emplace(fd, move(conn));
  }
  return true;
}

TCP_NAMESPACE_END
#endif
