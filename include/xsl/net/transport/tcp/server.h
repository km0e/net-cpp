#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_SERVER_H_
#  define _XSL_NET_TRANSPORT_TCP_SERVER_H_
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
  shared_ptr<Poller> poller;
  shared_ptr<HG> handler_generator;
};
template <TcpHandler H, TcpHandlerGenerator<H> HG>
TcpServerConfig<H, HG>::TcpServerConfig()
    : host("0.0.0.0"), port(8080), poller(nullptr), handler_generator(nullptr) {
  // for debug
  spdlog::set_pattern("[%D-%T][%^%l%$][c%t][%!] %v");
}

template <TcpHandler H, TcpHandlerGenerator<H> HG>
class TcpServer {
public:
  static unique_ptr<TcpServer<H, HG>> serve(TcpServerConfig<H, HG> config);
  TcpServer(TcpServer&&) = delete;
  TcpServer(int fd, shared_ptr<HG> handler_generator, shared_ptr<Poller> poller);
  ~TcpServer();
  IOM_EVENTS accept(int fd, IOM_EVENTS events);

private:
  int fd;
  // Handler is a function that takes a shared pointer to a Poller, an int, and an IOM_EVENTS enum
  // and returns a bool The handler is called when the server receives a connection
  shared_ptr<HG> handler_generator;
  shared_ptr<Poller> poller;
  ShareContainer<unordered_map<int, unique_ptr<TcpConn<H>>>> handlers;
};

template <TcpHandler H, TcpHandlerGenerator<H> HG>
TcpServer<H, HG>::TcpServer(int fd, shared_ptr<HG> handler_generator, shared_ptr<Poller> poller)
    : fd(fd), handler_generator(handler_generator), poller(poller), handlers() {
  poller->subscribe(fd, IOM_EVENTS::IN,
                    [this](int fd, IOM_EVENTS events) { return this->accept(fd, events); });
}

template <TcpHandler H, TcpHandlerGenerator<H> HG>
TcpServer<H, HG>::~TcpServer() {
  if (fd != -1) {
    close(fd);
  }
}

template <TcpHandler H, TcpHandlerGenerator<H> HG>
unique_ptr<TcpServer<H, HG>> TcpServer<H, HG>::serve(TcpServerConfig<H, HG> config) {
  TcpServerSockConfig cfg{};
  cfg.max_connections = config.max_connections;
  int server_fd = create_tcp_server(config.host.data(), config.port, cfg);
  if (server_fd == -1) {
    SPDLOG_ERROR("Failed to create server");
    return nullptr;
  }
  return make_unique<TcpServer<H, HG>>(server_fd, config.handler_generator, config.poller);
}

template <TcpHandler H, TcpHandlerGenerator<H> HG>
IOM_EVENTS TcpServer<H, HG>::accept(int fd, IOM_EVENTS events) {
  SPDLOG_TRACE("start accept");
  if ((events & IOM_EVENTS::IN) == IOM_EVENTS::IN) {
    SPDLOG_INFO("New connection");
    sockaddr addr;
    socklen_t addr_len = sizeof(addr);
    int client_fd = ::accept(fd, &addr, &addr_len);
    if (client_fd == -1) {  // todo: handle error
      return IOM_EVENTS::IN;
    }
    if (!set_non_blocking(client_fd)) {
      SPDLOG_WARN("[TcpServer::<lambda::handler>] Failed to set non-blocking");
      close(client_fd);
      return IOM_EVENTS::IN;
    }
    auto tcp_conn = make_unique<TcpConn<H>>(client_fd, this->poller, (*this->handler_generator)());
    tcp_conn->recv(client_fd, IOM_EVENTS::IN);
    if (tcp_conn->valid()) {
      SPDLOG_DEBUG("New connection established");
      // TODO: drop connection
      this->handlers.lock()->emplace(client_fd, std::move(tcp_conn));
    }
    SPDLOG_TRACE("accept done");
    return IOM_EVENTS::IN;
  }
  SPDLOG_TRACE("there is no IN event");
  return IOM_EVENTS::NONE;
}

TCP_NAMESPACE_END
#endif
