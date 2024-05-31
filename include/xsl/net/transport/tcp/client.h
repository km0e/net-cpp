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
template <TcpHandlerLike H>
class TcpClientConfig {
public:
  TcpClientConfig() : poller(nullptr) {
    // for debug
    // spdlog::set_pattern("[%D-%T][%^%l%$][%t][%!] %v");
  }
  int max_connections = MAX_CONNECTIONS;
  shared_ptr<Poller> poller;
};
template <TcpHandlerLike H>
class TcpClient {
public:
  static unique_ptr<TcpClient<H>> dial(TcpClientConfig<H> config) {
    return make_unique<TcpClient<H>>(config);
  }
  TcpClient(TcpClient&&) = delete;
  TcpClient(TcpClientConfig<H> config) : config(config), handlers() {}
  ~TcpClient() {
    // TODO : close all connections
  }

  bool send(const char* ip, const char* port, H&& handler) {
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

private:
  // Handler is a function that takes a shared pointer to a Poller, an int, and an IOM_EVENTS enum
  // and returns a bool The handler is called when the server receives a connection
  TcpClientConfig<H> config;
  ShareContainer<unordered_map<int, unique_ptr<TcpConn<H>>>> handlers;
};

TCP_NAMESPACE_END
#endif
