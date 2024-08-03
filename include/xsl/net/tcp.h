#pragma once
#ifndef XSL_NET_TCP
#  define XSL_NET_TCP
#  include "xsl/net/def.h"
#  include "xsl/net/transport/accept.h"
#  include "xsl/sys.h"
#  include "xsl/sys/net/socket.h"

#  include <expected>
#  include <memory>
NET_NB
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
    auto copy_poller = poller;
    return tcp_serv<Flags...>(host, port)
        .and_then([&copy_poller](auto &&skt) {
          return transport::Acceptor::create(*copy_poller, std::move(skt));
        })
        .transform([copy_poller = std::move(copy_poller)](auto &&ac) {
          return TcpServer{std::move(copy_poller), std::move(ac)};
        });
  }
  template <class... Flags>
  static std::expected<TcpServer, std::error_condition> create(std::shared_ptr<Poller> &&poller,
                                                               const char *host, const char *port) {
    return tcp_serv<Flags...>(host, port)
        .and_then(
            [&poller](auto &&skt) { return transport::Acceptor::create(*poller, std::move(skt)); })
        .transform([poller = std::move(poller)](auto &&ac) {
          return TcpServer{std::move(poller), std::move(ac)};
        });
  }
  template <class P, class... Args>
  TcpServer(P &&poller, Args &&...args)
      : poller(std::forward<P>(poller)), _ac(std::forward<Args>(args)...) {}
  TcpServer(TcpServer &&) = default;
  template <class Executor = coro::ExecutorBase>
  decltype(auto) accept(sys::net::SockAddr *addr) noexcept {
    return this->_ac.accept<Executor>(addr).transform([this](auto &&res) {
      return res.transform([this](auto &&skt) { return std::move(skt).async(*this->poller); });
    });
  }

private:
  std::shared_ptr<Poller> poller;
  transport::Acceptor _ac;
};
NET_NE
#endif
