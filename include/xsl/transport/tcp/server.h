#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_SERVER_H_
#  define _XSL_NET_TRANSPORT_TCP_SERVER_H_
#  include "xsl/sync/poller.h"
#  include "xsl/transport/tcp/conn.h"
#  include "xsl/transport/tcp/def.h"
#  include "xsl/transport/utils.h"
#  include "xsl/utils/utils.h"
#  include "xsl/wheel/wheel.h"

#  include <spdlog/spdlog.h>

TCP_NAMESPACE_BEGIN

template <Handler H, HandlerGenerator<H> HG>
class TcpServer {
public:
  TcpServer(TcpConfig config = {});
  ~TcpServer();
  bool serve(const char* host, int port);
  void set_poller(wheel::shared_ptr<sync::Poller> poller);
  bool valid();
  void set_handler_generator(wheel::shared_ptr<HG> handler_generator);
  void set_max_connections(int max_connections);

private:
  sync::IOM_EVENTS proxy_handler(int fd, sync::IOM_EVENTS events);
  sync::IOM_EVENTS accept_handler(int fd, sync::IOM_EVENTS events);
  int server_fd;
  TcpConfig config;
  // Handler is a function that takes a shared pointer to a Poller, an int, and an IOM_EVENTS enum
  // and returns a bool The handler is called when the server receives a connection
  wheel::shared_ptr<HG> handler_generator;
  wheel::ConcurrentHashMap<int, wheel::unique_ptr<TcpConn<H>>> handlers;
  wheel::shared_ptr<sync::Poller> poller;
};

template <Handler H, HandlerGenerator<H> HG>
TcpServer<H, HG>::TcpServer(TcpConfig config)
    : server_fd(-1), config(config), handler_generator(nullptr), handlers(), poller(nullptr) {
  spdlog::set_pattern("[%D-%T][%^%l%$][%t][%!] %v");
  // SPDLOG_ERROR("{}",(int)spdlog::get_level());
}

template <Handler H, HandlerGenerator<H> HG>
TcpServer<H, HG>::~TcpServer() {
  if (this->valid()) {
    close(this->server_fd);
  }
}
template <Handler H, HandlerGenerator<H> HG>
bool TcpServer<H, HG>::serve(const char* host, int port) {
  SPDLOG_TRACE("Starting server on {}:{}", host, port);
  this->server_fd = create_tcp_server(host, port, this->config);
  if (this->valid()) {
    auto res = this->poller->subscribe(
        this->server_fd, sync::IOM_EVENTS::IN,
        [this](int fd, sync::IOM_EVENTS events) { return this->accept_handler(fd, events); });
    if (!res) {
      SPDLOG_ERROR("Failed to register handler");
      return false;
    }
  }
  SPDLOG_INFO("Server started on {}:{}", host, port);
  return this->valid();
}

template <Handler H, HandlerGenerator<H> HG>
void TcpServer<H, HG>::set_poller(wheel::shared_ptr<sync::Poller> poller) {
  this->poller = poller;
}

template <Handler H, HandlerGenerator<H> HG>
bool TcpServer<H, HG>::valid() {
  return this->server_fd != -1 && this->poller != nullptr;
}
template <Handler H, HandlerGenerator<H> HG>
void TcpServer<H, HG>::set_handler_generator(wheel::shared_ptr<HG> handler_generator) {
  this->handler_generator = handler_generator;
}
template <Handler H, HandlerGenerator<H> HG>
void TcpServer<H, HG>::set_max_connections(int max_connections) {
  this->config.max_connections = max_connections;
}

// template <Handler H, HandlerGenerator<H> HG>
// sync::IOM_EVENTS TcpServer<H, HG>::proxy_handler(int fd, sync::IOM_EVENTS events) {
//   sync::IOM_EVENTS res = sync::IOM_EVENTS::NONE;
//   // TODO: handle state
//   if ((events & sync::IOM_EVENTS::IN) == sync::IOM_EVENTS::IN) {
//     auto it = this->handlers.find(fd);
//     if (it == this->handlers.end()) {
//       SPDLOG_ERROR("[TcpServer::proxy_handler] No handler found for fd: {}", fd);
//       return sync::IOM_EVENTS::NONE;
//     }
//     res |= it->second.recv();
//   }
//   if ((events & sync::IOM_EVENTS::OUT) == sync::IOM_EVENTS::OUT) {
//     auto it = this->handlers.find(fd);
//     if (it == this->handlers.end()) {
//       SPDLOG_ERROR("[TcpServer::proxy_handler] No handler found for fd: {}", fd);
//       return sync::IOM_EVENTS::NONE;
//     }
//     res |= it->second.send();
//   }
//   return res;
// }
template <Handler H, HandlerGenerator<H> HG>
sync::IOM_EVENTS TcpServer<H, HG>::accept_handler(int fd, sync::IOM_EVENTS events) {
  SPDLOG_TRACE("start accept");
  if ((events & sync::IOM_EVENTS::IN) == sync::IOM_EVENTS::IN) {
    SPDLOG_INFO("New connection");
    sockaddr addr;
    socklen_t addr_len = sizeof(addr);
    int client_fd = accept(fd, &addr, &addr_len);
    if (client_fd == -1) {  // todo: handle error
      return sync::IOM_EVENTS::IN;
    }
    if (!set_non_blocking(client_fd)) {
      SPDLOG_WARN("[TcpServer::<lambda::handler>] Failed to set non-blocking");
      close(client_fd);
      return sync::IOM_EVENTS::IN;
    }
    auto tcp_conn
        = TcpConn<H>::make_tcp_conn(client_fd, this->poller, (*this->handler_generator)());
    tcp_conn->recv();
    if (tcp_conn->valid()) {
      SPDLOG_DEBUG("New connection established");
      // TODO: drop connection
      this->handlers.lock()->try_emplace(client_fd, std::move(tcp_conn));
    }
    SPDLOG_TRACE("accept done");
    return sync::IOM_EVENTS::IN;
  }
  SPDLOG_TRACE("there is no IN event");
  return sync::IOM_EVENTS::NONE;
}

TCP_NAMESPACE_END
#endif
