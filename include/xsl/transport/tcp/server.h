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
#  include <unistd.h>

TCP_NAMESPACE_BEGIN
template <TcpHandler H, HandlerGenerator<H> HG>
class TcpServerConfig {
public:
  TcpServerConfig();
  int max_connections = MAX_CONNECTIONS;
  wheel::string_view host;
  int port;
  wheel::shared_ptr<sync::Poller> poller;
  wheel::shared_ptr<HG> handler_generator;
};
template <TcpHandler H, HandlerGenerator<H> HG>
TcpServerConfig<H, HG>::TcpServerConfig() {
  // for debug
  spdlog::set_pattern("[%D-%T][%^%l%$][%t][%!] %v");
}
// template <TcpHandler H, HandlerGenerator<H> HG>
// class TcpServerImpl {
// public:
//   static wheel::unique_ptr<TcpServerImpl<H, HG>> make_tcp_server_impl(
//       int fd, wheel::shared_ptr<HG> handler_generator, wheel::shared_ptr<sync::Poller> poller) {
//     auto impl = wheel::make_unique<TcpServerImpl<H, HG>>(handler_generator, poller);
//     auto res = poller->subscribe(fd, sync::IOM_EVENTS::IN,
//                                  [server = impl.get()](int fd, sync::IOM_EVENTS events) {
//                                    return server->accept(fd, events);
//                                  });
//     if (!res) {
//       SPDLOG_ERROR("Failed to register handler");
//       return nullptr;
//     }
//     return impl;
//   }
//   // Don't directly use the constructor, use make_tcp_server_impl instead
//   TcpServerImpl(wheel::shared_ptr<HG> handler_generator, wheel::shared_ptr<sync::Poller> poller);
//   sync::IOM_EVENTS accept(int fd, sync::IOM_EVENTS);

// public:
//   wheel::shared_ptr<HG> handler_generator;
//   wheel::ConcurrentHashMap<int, wheel::unique_ptr<TcpConn<H>>> handlers;
//   wheel::shared_ptr<sync::Poller> poller;
// };
// template <TcpHandler H, HandlerGenerator<H> HG>
// class TcpServerBuilder {
// public:
//   TcpServerBuilder(wheel::shared_ptr<HG> handler_generator, wheel::shared_ptr<sync::Poller>
//   poller);

// public:
//   wheel::shared_ptr<HG> handler_generator;
//   wheel::shared_ptr<sync::Poller> poller;
// };

// template <TcpHandler H, HandlerGenerator<H> HG>
// TcpServerImpl<H, HG>::TcpServerImpl(wheel::shared_ptr<HG> handler_generator,
//                                     wheel::shared_ptr<sync::Poller> poller)
//     : handler_generator(handler_generator), poller(poller) {}
// template <TcpHandler H, HandlerGenerator<H> HG>
// sync::IOM_EVENTS TcpServerImpl<H, HG>::accept(int fd, sync::IOM_EVENTS events) {
//   SPDLOG_TRACE("start accept");
//   if ((events & sync::IOM_EVENTS::IN) == sync::IOM_EVENTS::IN) {
//     SPDLOG_INFO("New connection");
//     sockaddr addr;
//     socklen_t addr_len = sizeof(addr);
//     int client_fd = accept(fd, &addr, &addr_len);
//     if (client_fd == -1) {  // todo: handle error
//       return sync::IOM_EVENTS::IN;
//     }
//     if (!set_non_blocking(client_fd)) {
//       SPDLOG_WARN("[TcpServer::<lambda::handler>] Failed to set non-blocking");
//       close(client_fd);
//       return sync::IOM_EVENTS::IN;
//     }
//     auto tcp_conn
//         = TcpConn<H>::make_tcp_conn(client_fd, this->poller, (*this->handler_generator)());
//     tcp_conn->recv(client_fd, sync::IOM_EVENTS::IN);
//     if (tcp_conn->valid()) {
//       SPDLOG_DEBUG("New connection established");
//       // TODO: drop connection
//       this->handlers.lock()->try_emplace(client_fd, std::move(tcp_conn));
//     }
//     SPDLOG_TRACE("accept done");
//     return sync::IOM_EVENTS::IN;
//   }
//   SPDLOG_TRACE("there is no IN event");
//   return sync::IOM_EVENTS::NONE;
// }

template <TcpHandler H, HandlerGenerator<H> HG>
class TcpServer {
public:
  static wheel::unique_ptr<TcpServer<H, HG>> serve(TcpServerConfig<H, HG> config);
  TcpServer(TcpServer&&) = delete;
  TcpServer(int fd, wheel::shared_ptr<HG> handler_generator,
            wheel::shared_ptr<sync::Poller> poller);
  ~TcpServer();
  void set_poller(wheel::shared_ptr<sync::Poller> poller);
  sync::IOM_EVENTS accept(int fd, sync::IOM_EVENTS events);
  void set_handler_generator(wheel::shared_ptr<HG> handler_generator);
  void set_max_connections(int max_connections);

private:
  int fd;
  // Handler is a function that takes a shared pointer to a Poller, an int, and an IOM_EVENTS enum
  // and returns a bool The handler is called when the server receives a connection
  wheel::shared_ptr<HG> handler_generator;
  wheel::ConcurrentHashMap<int, wheel::unique_ptr<TcpConn<H>>> handlers;
  wheel::shared_ptr<sync::Poller> poller;
};

template <TcpHandler H, HandlerGenerator<H> HG>
TcpServer<H, HG>::TcpServer(int fd, wheel::shared_ptr<HG> handler_generator,
                            wheel::shared_ptr<sync::Poller> poller)
    : fd(fd), handler_generator(handler_generator), poller(poller) {
  poller->subscribe(fd, sync::IOM_EVENTS::IN,
                    [this](int fd, sync::IOM_EVENTS events) { return this->accept(fd, events); });
}

template <TcpHandler H, HandlerGenerator<H> HG>
TcpServer<H, HG>::~TcpServer() {
  if (fd != -1) {
    close(fd);
  }
}

template <TcpHandler H, HandlerGenerator<H> HG>
wheel::unique_ptr<TcpServer<H, HG>> TcpServer<H, HG>::serve(TcpServerConfig<H, HG> config) {
  TcpConfig cfg{};
  cfg.max_connections = config.max_connections;
  int server_fd = create_tcp_server(config.host.data(), config.port, cfg);
  if (server_fd == -1) {
    SPDLOG_ERROR("Failed to create server");
    return nullptr;
  }
  return wheel::make_unique<TcpServer<H, HG>>(server_fd, config.handler_generator, config.poller);
}

template <TcpHandler H, HandlerGenerator<H> HG>
sync::IOM_EVENTS TcpServer<H, HG>::accept(int fd, sync::IOM_EVENTS events) {
  SPDLOG_TRACE("start accept");
  if ((events & sync::IOM_EVENTS::IN) == sync::IOM_EVENTS::IN) {
    SPDLOG_INFO("New connection");
    sockaddr addr;
    socklen_t addr_len = sizeof(addr);
    int client_fd = ::accept(fd, &addr, &addr_len);
    if (client_fd == -1) {  // todo: handle error
      return sync::IOM_EVENTS::IN;
    }
    if (!set_non_blocking(client_fd)) {
      SPDLOG_WARN("[TcpServer::<lambda::handler>] Failed to set non-blocking");
      close(client_fd);
      return sync::IOM_EVENTS::IN;
    }
    auto tcp_conn
        = wheel::make_unique<TcpConn<H>>(client_fd, this->poller, (*this->handler_generator)());
    tcp_conn->recv(client_fd, sync::IOM_EVENTS::IN);
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
// template <TcpHandler H, HandlerGenerator<H> HG>
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
// template <TcpHandler H, HandlerGenerator<H> HG>
// sync::IOM_EVENTS TcpServer<H, HG>::accept_handler(int fd, sync::IOM_EVENTS events) {
//   SPDLOG_TRACE("start accept");
//   if ((events & sync::IOM_EVENTS::IN) == sync::IOM_EVENTS::IN) {
//     SPDLOG_INFO("New connection");
//     sockaddr addr;
//     socklen_t addr_len = sizeof(addr);
//     int client_fd = accept(fd, &addr, &addr_len);
//     if (client_fd == -1) {  // todo: handle error
//       return sync::IOM_EVENTS::IN;
//     }
//     if (!set_non_blocking(client_fd)) {
//       SPDLOG_WARN("[TcpServer::<lambda::handler>] Failed to set non-blocking");
//       close(client_fd);
//       return sync::IOM_EVENTS::IN;
//     }
//     auto tcp_conn
//         = TcpConn<H>::make_tcp_conn(client_fd, this->poller, (*this->handler_generator)());
//     tcp_conn->recv(client_fd, sync::IOM_EVENTS::IN);
//     if (tcp_conn->valid()) {
//       SPDLOG_DEBUG("New connection established");
//       // TODO: drop connection
//       this->handlers.lock()->try_emplace(client_fd, std::move(tcp_conn));
//     }
//     SPDLOG_TRACE("accept done");
//     return sync::IOM_EVENTS::IN;
//   }
//   SPDLOG_TRACE("there is no IN event");
//   return sync::IOM_EVENTS::NONE;
// }

TCP_NAMESPACE_END
#endif
