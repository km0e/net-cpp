#pragma once
#ifndef XSL_NET_TRANSPORT_TCP_SERVER
#  define XSL_NET_TRANSPORT_TCP_SERVER
#  include "xsl/coro/task.h"
#  include "xsl/net/transport/resolve.h"
#  include "xsl/net/transport/tcp/accept.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/net/transport/tcp/stream.h"
#  include "xsl/sync.h"
#  include "xsl/sys/net/socket.h"

#  include <expected>
#  include <memory>

TCP_NB

template <class... Flags>
std::expected<sys::net::Socket, std::error_condition> tcp_serv(const char *host, const char *port) {
  auto addr = Resolver{}.resolve<Flags...>(host, port, SERVER_FLAGS);
  if (!addr) {
    return std::unexpected(addr.error());
  }
  auto bres = bind(addr.value());
  if (!bres) {
    return std::unexpected(bres.error());
  }
  auto lres = listen(bres.value());
  if (!lres) {
    return std::unexpected(lres.error());
  }
  return sys::net::Socket{std::move(bres.value())};
}

class TcpServer {
public:
  template <class... Flags>
  static std::expected<TcpServer, std::error_condition> create(std::shared_ptr<Poller> poller,
                                                               const char *host, const char *port) {
    auto skt = tcp_serv<Flags...>(host, port);
    if (!skt) {
      return std::unexpected(skt.error());
    }
    return TcpServer{std::move(poller), std::move(skt.value())};
  }
  TcpServer(const std::shared_ptr<Poller> &poller, Socket &&socket)
      : poller(poller), acceptor{std::move(socket), this->poller} {}
  TcpServer(TcpServer &&) = default;
  coro::Task<std::expected<TcpStream, std::error_condition>> gen() {
    co_return (co_await acceptor).transform([this](auto &&res) -> TcpStream {
      auto [skt, _] = std::move(res);
      return {std::move(skt), this->poller};
    });
  }

private:
  std::shared_ptr<Poller> poller;
  Acceptor acceptor;
};

TCP_NE
#endif
