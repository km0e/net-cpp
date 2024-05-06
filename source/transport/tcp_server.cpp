#include <spdlog/spdlog.h>
#include <xsl/sync/poller.h>
#include <xsl/transport/tcp_server.h>
#include <xsl/transport/transport.h>
#include <xsl/transport/utils.h>
#include <xsl/utils/utils.h>

#include <cstddef>

#define MAX_CONNECTIONS 10

TRANSPORT_NAMESPACE_BEGIN

HandleState::HandleState(sync::IOM_EVENTS events, HandleHint hint) : events(events), hint(hint) {}
HandleState::HandleState() : events(sync::IOM_EVENTS::NONE), hint(HandleHint::NONE) {}
HandleState::~HandleState() {}

TcpConn::TcpConn(int fd, wheel::shared_ptr<sync::Poller> poller, Handler&& handler)
    : fd(fd), poller(poller), handler(handler) {}

TcpConn::~TcpConn() { close(this->fd); }

sync::IOM_EVENTS TcpConn::send() {
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
sync::IOM_EVENTS TcpConn::recv() {
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

TcpServer::TcpServer(TcpConfig config) : server_fd(-1), config(config), poller(nullptr) {}

bool TcpServer::serve(const char* host, int port) {
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
sync::IOM_EVENTS TcpServer::accept_handler(int fd, sync::IOM_EVENTS events) {
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

TcpServer::~TcpServer() {
  if (this->valid()) {
    close(this->server_fd);
  }
}
void TcpServer::set_poller(wheel::shared_ptr<sync::Poller> poller) { this->poller = poller; }

bool TcpServer::valid() { return this->server_fd != -1 && this->poller != nullptr; }
void TcpServer::set_handler_generator(wheel::shared_ptr<HandlerGenerator> handler_generator) {
  this->handler_generator = handler_generator;
}
void TcpServer::set_max_connections(int max_connections) {
  this->config.max_connections = max_connections;
}

sync::IOM_EVENTS TcpServer::proxy_handler(int fd, sync::IOM_EVENTS events) {
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

TRANSPORT_NAMESPACE_END
