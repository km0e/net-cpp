#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_SERVER_H_
#  define _XSL_NET_TRANSPORT_TCP_SERVER_H_
#  include <spdlog/spdlog.h>
#  include <xsl/sync/poller.h>
#  include <xsl/transport/transport.h>
#  include <xsl/transport/utils.h>
#  include <xsl/utils/utils.h>
#  include <xsl/utils/wheel/wheel.h>
TRANSPORT_NAMESPACE_BEGIN

enum class HandleHint {
  NONE = 0,
  // Hint that the param data is a pointer to a string that should be sent
  WRITE = 2,
};

class HandleState {
public:
  HandleState();
  HandleState(sync::IOM_EVENTS events, HandleHint hint);
  ~HandleState();
  sync::IOM_EVENTS events;
  HandleHint hint;
};
template <class T>
concept Handler = requires(T t, int i, wheel::string& s) {
  { t(i, s) } -> wheel::same_as<HandleState>;
};

template <Handler H>
class TcpConn {
public:
  TcpConn(TcpConn&&) = default;
  TcpConn(int fd, wheel::shared_ptr<sync::Poller> poller, H&& handler);
  ~TcpConn();
  sync::IOM_EVENTS send();
  sync::IOM_EVENTS recv();

private:
  int fd;
  wheel::shared_ptr<sync::Poller> poller;
  H handler;
  wheel::string send_buffer;
};
template <class T, class H>
concept HandlerGenerator = Handler<H> && requires(T t, H h) {
  { t() } -> wheel::same_as<H>;
};
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
  wheel::unordered_map<int, TcpConn<H>> handlers;
  wheel::shared_ptr<sync::Poller> poller;
};
template <Handler H>
TcpConn<H>::TcpConn(int fd, wheel::shared_ptr<sync::Poller> poller, H&& handler)
    : fd(fd), poller(poller), handler(handler) {}

template <Handler H>
TcpConn<H>::~TcpConn() {
  close(this->fd);
}

template <Handler H>
sync::IOM_EVENTS TcpConn<H>::send() {
  spdlog::trace("[TcpConn::send]");
  wheel::string data;
  auto state = HandleState{};
  while (true) {
    state = this->handler(1, data);
    this->send_buffer += data;
    while (!this->send_buffer.empty()) {
      ssize_t n = ::send(this->fd, this->send_buffer.c_str(), this->send_buffer.size(), 0);
      if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          spdlog::debug("[TcpConn::send] send over");
        } else {
          spdlog::error("[TcpConn::send] Failed to send data");
        }
        break;
      } else if ((size_t)n == this->send_buffer.size()) {
        break;
      }
      this->send_buffer = this->send_buffer.substr(n);
    }
    if ((state.events & sync::IOM_EVENTS::OUT) != sync::IOM_EVENTS::OUT) {
      break;
    }
  }
  return state.events;
}
template <Handler H>
sync::IOM_EVENTS TcpConn<H>::recv() {
  spdlog::trace("[TcpConn::recv]");
  spdlog::trace("[read] Reading from fd: {}", fd);
  wheel::string data;
  data.clear();
  data.reserve(1024);
  char buf[1024];
  ssize_t n;
  while ((n = ::recv(fd, buf, sizeof(buf), 0)) > 0) {
    data.append(buf, n);
  }
  spdlog::debug("[read] data size: {}", data.size());
  if (n == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      spdlog::debug("[read] recv over");
    } else {
      spdlog::error("[read] Failed to recv data");
      return sync::IOM_EVENTS::NONE;
    }
  }
  if (n == 0) {
    spdlog::debug("[read] recv over");
  }
  auto state = this->handler(0, data);
  if (state.hint == HandleHint::WRITE) {
    spdlog::debug("[TcpConn::recv] Handling write hint");
    this->send_buffer += data;
  }
  return state.events;
}

template <Handler H, HandlerGenerator<H> HG>
TcpServer<H, HG>::TcpServer(TcpConfig config)
    : server_fd(-1), config(config), poller(nullptr), handler_generator(nullptr), handlers() {}

template <Handler H, HandlerGenerator<H> HG>
TcpServer<H, HG>::~TcpServer() {
  if (this->valid()) {
    close(this->server_fd);
  }
}
template <Handler H, HandlerGenerator<H> HG>
bool TcpServer<H, HG>::serve(const char* host, int port) {
  this->server_fd = create_tcp_server(host, port, this->config);
  if (this->valid()) {
    auto res = this->poller->register_handler(
        this->server_fd, sync::IOM_EVENTS::IN,
        [this](int fd, sync::IOM_EVENTS events) { return this->accept_handler(fd, events); });
    if (!res) {
      spdlog::error("[TcpServer::serve] Failed to register handler");
      return false;
    }
  }
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

template <Handler H, HandlerGenerator<H> HG>
sync::IOM_EVENTS TcpServer<H, HG>::proxy_handler(int fd, sync::IOM_EVENTS events) {
  sync::IOM_EVENTS res = sync::IOM_EVENTS::NONE;
  // TODO: handle state
  if ((events & sync::IOM_EVENTS::IN) == sync::IOM_EVENTS::IN) {
    auto it = this->handlers.find(fd);
    if (it == this->handlers.end()) {
      spdlog::error("[TcpServer::proxy_handler] No handler found for fd: {}", fd);
      return sync::IOM_EVENTS::NONE;
    }
    res |= it->second.recv();
  }
  if ((events & sync::IOM_EVENTS::OUT) == sync::IOM_EVENTS::OUT) {
    auto it = this->handlers.find(fd);
    if (it == this->handlers.end()) {
      spdlog::error("[TcpServer::proxy_handler] No handler found for fd: {}", fd);
      return sync::IOM_EVENTS::NONE;
    }
    res |= it->second.send();
  }
  return res;
}
template <Handler H, HandlerGenerator<H> HG>
sync::IOM_EVENTS TcpServer<H, HG>::accept_handler(int fd, sync::IOM_EVENTS events) {
  if ((events & sync::IOM_EVENTS::IN) == sync::IOM_EVENTS::IN) {
    spdlog::info("[TcpServer::<lambda::handler>] New connection");
    sockaddr addr;
    socklen_t addr_len = sizeof(addr);
    int client_fd = accept(fd, &addr, &addr_len);
    if (client_fd == -1) {  // todo: handle error
      return sync::IOM_EVENTS::IN;
    }
    if (!set_non_blocking(client_fd)) {
      spdlog::warn("[TcpServer::<lambda::handler>] Failed to set non-blocking");
      close(client_fd);
      return sync::IOM_EVENTS::IN;
    }
    auto tcp_conn = TcpConn(client_fd, this->poller, (*this->handler_generator)());
    auto events = tcp_conn.recv();
    if ((events & sync::IOM_EVENTS::OUT) == sync::IOM_EVENTS::OUT) {
      spdlog::debug("[TcpServer::<lambda::handler>] Sending data");
      events = tcp_conn.send();
    }
    if (events == sync::IOM_EVENTS::NONE) {
      spdlog::debug("[TcpServer::<lambda::handler>] One-time connection");
      return sync::IOM_EVENTS::IN;
    }
    this->handlers.try_emplace(client_fd, std::move(tcp_conn));
    if (!poller->register_handler(client_fd, events, [this](int fd, sync::IOM_EVENTS events) {
          return this->proxy_handler(fd, events);
        })) {
      close(client_fd);
      return sync::IOM_EVENTS::IN;
    }
  }
  return sync::IOM_EVENTS::NONE;
}

TRANSPORT_NAMESPACE_END
#endif
