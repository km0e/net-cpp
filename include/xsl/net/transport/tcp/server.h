#pragma once
#ifndef XSL_NET_TRANSPORT_TCP_SERVER
#  define XSL_NET_TRANSPORT_TCP_SERVER
#  include "xsl/coro/task.h"
#  include "xsl/logctl.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/sync.h"
#  include "xsl/sys.h"
#  include "xsl/sys/net/socket.h"

#  include <expected>
#  include <memory>
#  include <system_error>
#  include <tuple>
#  include <utility>

TCP_NB

template <class... Flags>
std::expected<sys::net::Socket, std::error_condition> tcp_serv(const char *host, const char *port) {
  auto addr = xsl::net::Resolver{}.resolve<Flags...>(host, port, xsl::net::SERVER_FLAGS);
  if (!addr) {
    return std::unexpected(addr.error());
  }
  auto bres = sys::tcp::bind(*addr);
  if (!bres) {
    return std::unexpected(bres.error());
  }
  auto lres = sys::tcp::listen(*bres);
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
    LOG4("Start listening on {}:{}", host, port);
    return tcp_serv<Flags...>(host, port).transform([&poller](auto &&skt) {
      auto copy_poller = poller;
      auto [r, w] = std::move(skt).split();
      auto async_r = std::move(r).async(copy_poller);
      return TcpServer{std::move(copy_poller), std::move(async_r)};
    });
  }
  template <class... Flags>
  static std::expected<TcpServer, std::error_condition> create(std::shared_ptr<Poller> &&poller,
                                                               const char *host, const char *port) {
    return tcp_serv<Flags...>(host, port).transform([&poller](auto &&skt) {
      return TcpServer{std::move(poller), std::move(skt)};
    });
  }
  TcpServer(const std::shared_ptr<Poller> &poller, sys::io::AsyncReadDevice &&dev)
      : poller(poller), dev(std::move(dev)) {}
  TcpServer(std::shared_ptr<Poller> &&poller, sys::io::AsyncReadDevice &&dev)
      : poller(std::move(poller)), dev(std::move(dev)) {}
  TcpServer(TcpServer &&) = default;
  coro::Task<
      std::expected<std::tuple<sys::io::AsyncReadWriteDevice, sys::net::SockAddr>, std::errc>>
  accept() noexcept {
    while (true) {
      auto res = sys::tcp::accept(this->dev.raw());
      if (res) {
        auto [sock, addr] = std::move(*res);
        auto async_dev = std::move(sock).async(this->poller);
        co_return std::make_tuple(std::move(async_dev), std::move(addr));
      } else if (res.error() == std::errc::resource_unavailable_try_again
                 || res.error() == std::errc::operation_would_block) {
        if (!co_await this->dev.sem()) {
          co_return std::unexpected{std::errc::operation_canceled};
        }
      } else {
        co_return std::unexpected{res.error()};
      }
    }
  }

private:
  std::shared_ptr<Poller> poller;
  sys::io::AsyncReadDevice dev;
};

TCP_NE
#endif
