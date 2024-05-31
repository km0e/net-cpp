#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_SERVER_H_
#  define _XSL_NET_TRANSPORT_TCP_SERVER_H_
#  include "xsl/net/sync.h"
#  include "xsl/net/sync/poller.h"
#  include "xsl/net/transport/tcp/conn.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/net/transport/tcp/utils.h"
#  include "xsl/utils.h"
#  include "xsl/wheel.h"

#  include <spdlog/spdlog.h>
#  include <unistd.h>

TCP_NAMESPACE_BEGIN
template <TcpHandler H, TcpHandlerGenerator<H> HG>
class TcpServerConfig {
public:
  TcpServerConfig();
  int max_connections = MAX_CONNECTIONS;
  string_view host;
  int port;
  int fd = -1;
  shared_ptr<Poller> poller;
  shared_ptr<HG> handler_generator;
};
template <TcpHandler H, TcpHandlerGenerator<H> HG>
TcpServerConfig<H, HG>::TcpServerConfig()
    : host("0.0.0.0"), port(8080), poller(nullptr), handler_generator(nullptr) {
}

template <TcpHandler H, TcpHandlerGenerator<H> HG>
class TcpServer {
public:
  static unique_ptr<TcpServer<H, HG>> serve(TcpServerConfig<H, HG> config);
  TcpServer(TcpServer&&) = delete;
  TcpServer(TcpServerConfig<H, HG> config) : config(config), handlers() {}
  ~TcpServer() {
    if (config.fd != -1) {
      close(config.fd);
    }
  }
  PollHandleHint operator()(int fd, IOM_EVENTS events) {
    SPDLOG_TRACE("start accept");
    if ((events & IOM_EVENTS::IN) == IOM_EVENTS::IN) {
      SPDLOG_INFO("New connection");
      sockaddr addr;
      socklen_t addr_len = sizeof(addr);
      int client_fd = ::accept(fd, &addr, &addr_len);
      if (client_fd == -1) {  // todo: handle error
        return {PollHandleHintTag::NONE};
      }
      if (!set_non_blocking(client_fd)) {
        SPDLOG_WARN("[TcpServer::<lambda::handler>] Failed to set non-blocking");
        close(client_fd);
        return {PollHandleHintTag::NONE};
      }
      auto tcp_conn = sub_unique<TcpConn<H>>(this->config.poller, client_fd, IOM_EVENTS::IN,
                                             client_fd, (*this->config.handler_generator)());
      this->handlers.lock()->emplace(client_fd, std::move(tcp_conn));
      SPDLOG_TRACE("accept done");
      return {PollHandleHintTag::NONE};
    }
    SPDLOG_TRACE("there is no IN event");
    return {PollHandleHintTag::NONE};
  }

private:
  // Handler is a function that takes a shared pointer to a Poller, an int, and an IOM_EVENTS enum
  // and returns a bool The handler is called when the server receives a connection

  TcpServerConfig<H, HG> config;

  ShareContainer<unordered_map<int, unique_ptr<TcpConn<H>>>> handlers;
};

template <TcpHandler H, TcpHandlerGenerator<H> HG>
unique_ptr<TcpServer<H, HG>> TcpServer<H, HG>::serve(TcpServerConfig<H, HG> config) {
  TcpServerSockConfig cfg{};
  cfg.max_connections = config.max_connections;
  int server_fd = create_tcp_server(config.host.data(), config.port, cfg);
  if (server_fd == -1) {
    SPDLOG_ERROR("Failed to create server");
    return nullptr;
  }
  config.fd = server_fd;
  return sub_unique<TcpServer<H, HG>>(config.poller, server_fd, sync::IOM_EVENTS::IN, config);
}

TCP_NAMESPACE_END
#endif
