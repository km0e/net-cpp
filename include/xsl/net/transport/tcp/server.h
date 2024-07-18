#pragma once
#ifndef XSL_NET_TRANSPORT_TCP_SERVER
#  define XSL_NET_TRANSPORT_TCP_SERVER
#  include "xsl/logctl.h"
#  include "xsl/net/transport/resolve.h"
#  include "xsl/net/transport/tcp/accept.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/net/transport/tcp/stream.h"
#  include "xsl/net/transport/tcp/utils.h"
#  include "xsl/sync.h"
#  include "xsl/sys/net/socket.h"

#  include <expected>
#  include <memory>
#  include <utility>

TCP_NB

template <class... Flags>
std::expected<sys::net::Socket, std::error_condition> tcp_serv(const char *host, const char *port) {
  auto addr = Resolver{}.resolve<Flags...>(host, port, SERVER_FLAGS);
  if (!addr) {
    return std::unexpected(addr.error());
  }
  auto bres = tcp::bind(*addr);
  if (!bres) {
    return std::unexpected(bres.error());
  }
  auto lres = tcp::listen(*bres);
  if (!lres) {
    return std::unexpected(lres.error());
  }
  return sys::net::Socket{std::move(*bres)};
}

class TcpServer {
public:
  template <class... Flags>
  static std::expected<TcpServer, std::error_condition> create(
      const std::shared_ptr<Poller> &poller, const char *host, const char *port) {
    tcp_serv<Flags...>(host, port).transform([&poller](auto &&skt) {
      return TcpServer{poller, std::move(skt)};
    });
  }
  template <class... Flags>
  static std::expected<TcpServer, std::error_condition> create(std::shared_ptr<Poller> &&poller,
                                                               const char *host, const char *port) {
    tcp_serv<Flags...>(host, port).transform([&poller](auto &&skt) {
      return TcpServer{std::move(poller), std::move(skt)};
    });
  }
  TcpServer(const std::shared_ptr<Poller> &poller, Socket &&socket)
      : poller(poller), acceptor{std::move(socket), this->poller} {}
  TcpServer(std::shared_ptr<Poller> &&poller, Socket &&socket)
      : poller(std::move(poller)), acceptor{std::move(socket), this->poller} {}
  TcpServer(TcpServer &&) = default;
  decltype(auto) accept() {
    DEBUG("tcp server accept");
    return acceptor.accept().transform([this](auto &&exp) {
      return exp.transform([this](auto &&res) -> TcpStream {
        return {std::move(std::get<0>(std::move(res))), this->poller};
      });
    });
  }

private:
  std::shared_ptr<Poller> poller;
  Acceptor acceptor;
};

TCP_NE
#endif
