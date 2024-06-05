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

template <class T, class H>
concept TcpHandlerGeneratorLike = TcpHandlerLike<H> && requires(T t, H h, int fd) {
  { t(fd) } -> same_as<unique_ptr<H>>;
};

template <TcpHandlerLike H, TcpHandlerGeneratorLike<H> HG>
class TcpServerConfig {
public:
  TcpServerConfig() : sa4("0.0.0.0", "0"), fd(-1), poller(nullptr), handler_generator(nullptr) {}
  SockAddrV4View sa4;
  int fd = -1;
  shared_ptr<Poller> poller;
  shared_ptr<HG> handler_generator;
  int max_connections = MAX_CONNECTIONS;
  bool keep_alive = false;
  int recv_timeout = RECV_TIMEOUT;
};
template <TcpHandlerLike H, TcpHandlerGeneratorLike<H> HG>
class TcpServer {
public:
  static unique_ptr<TcpServer<H, HG>> serve(TcpServerConfig<H, HG> config) {
    TcpServerSockConfig cfg{};
    cfg.max_connections = config.max_connections;
    cfg.keep_alive = config.keep_alive;
    int server_fd = create_tcp_server(config.sa4, cfg);
    if (server_fd == -1) {
      SPDLOG_ERROR("Failed to create server, error: {}", strerror(errno));
      return nullptr;
    }
    config.fd = server_fd;
    return poll_add_unique<TcpServer<H, HG>>(config.poller, server_fd, sync::IOM_EVENTS::IN, config);
  }
  TcpServer(TcpServer&&) = delete;
  TcpServer(TcpServerConfig<H, HG> config)
      : config(config),
        tcp_conn_manager(TcpConnManagerConfig{config.poller, config.recv_timeout}) {}
  ~TcpServer() {
    if (config.fd != -1) {
      close(config.fd);
    }
  }
  PollHandleHint operator()(int fd, IOM_EVENTS events) {
    SPDLOG_TRACE("start accept");
    if ((events & IOM_EVENTS::IN) == IOM_EVENTS::IN) {
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
      auto handler = (*this->config.handler_generator)(client_fd);
      if (!handler) {
        SPDLOG_WARN("[TcpServer::<lambda::handler>] Failed to create handler");
        close(client_fd);
        return {PollHandleHintTag::NONE};
      }
      this->tcp_conn_manager.add(client_fd, std::move(handler));
      SPDLOG_TRACE("accept done");
      return {PollHandleHintTag::NONE};
    }
    SPDLOG_TRACE("there is no IN event");
    return {PollHandleHintTag::NONE};
  }
  bool stop() {
    if (config.fd != -1) {
      close(config.fd);
      config.fd = -1;
    }
    return true;
  }

private:
  // Handler is a function that takes a shared pointer to a Poller, an int, and an IOM_EVENTS enum
  // and returns a bool The handler is called when the server receives a connection

  TcpServerConfig<H, HG> config;
  TcpConnManager<H> tcp_conn_manager;
};
TCP_NAMESPACE_END
#endif
