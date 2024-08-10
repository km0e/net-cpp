#pragma once
#ifndef XSL_NET_TCP
#  define XSL_NET_TCP
#  include "xsl/coro/task.h"
#  include "xsl/feature.h"
#  include "xsl/net/def.h"
#  include "xsl/net/transport/accept.h"
#  include "xsl/sync/poller.h"
#  include "xsl/sys.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/socket.h"

#  include <expected>
#  include <memory>
XSL_NET_NB
template <class... Flags>
coro::Task<
    std::expected<sys::net::AsyncSocket<sys::net::SocketTraits<Flags...>>, std::error_condition>>
tcp_dial(const char *host, const char *port, sync::Poller &poller) {
  auto res = xsl::net::Resolver{}.resolve<Flags...>(host, port, xsl::net::CLIENT_FLAGS);
  if (!res) {
    co_return std::unexpected{res.error()};
  }
  auto conn_res = co_await sys::tcp::connect(*res, poller);
  if (!conn_res) {
    co_return std::unexpected{conn_res.error()};
  }
  co_return std::move(*conn_res);
}

template <class LowerLayer>
std::expected<sys::net::TcpSocket<LowerLayer>, std::error_condition> tcp_serv(const char *host,
                                                                              const char *port) {
  auto addr
      = xsl::net::Resolver{}.resolve<feature::Tcp<LowerLayer>>(host, port, xsl::net::SERVER_FLAGS);
  if (!addr) {
    return std::unexpected(addr.error());
  }
  auto bind_res = sys::tcp::bind(*addr);
  if (!bind_res) {
    return std::unexpected(bind_res.error());
  }
  auto lres = sys::tcp::listen(*bind_res);
  if (!lres) {
    return std::unexpected(lres.error());
  }
  return sys::net::TcpSocket<LowerLayer>{std::move(*bind_res)};
}

template <class LowerLayer>
class TcpServer;

template <uint8_t Version>
class TcpServer<feature::Ip<Version>> {
public:
  using lower_layer_type = feature::Ip<Version>;
  using io_dev_type = sys::net::AsyncTcpSocket<lower_layer_type>;
  using in_dev_type = io_dev_type::template rebind_type<feature::In>;
  using out_dev_type = io_dev_type::template rebind_type<feature::Out>;

  static std::expected<TcpServer, std::error_condition> create(
      std::string_view host, std::string_view port, const std::shared_ptr<Poller> &poller) {
    LOG5("Start listening on {}:{}", host, port);
    auto copy_poller = poller;
    auto skt = tcp_serv<lower_layer_type>(host.data(), port.data());
    if (!skt) {
      return std::unexpected(skt.error());
    }
    auto ac = transport::Acceptor<lower_layer_type>::create(*copy_poller, std::move(*skt));
    if (!ac) {
      return std::unexpected(ac.error());
    }
    return TcpServer{host, port, std::move(copy_poller), std::move(*ac)};
  }

  template <class P, class... Args>
  TcpServer(std::string_view host, std::string_view port, P &&poller, Args &&...args)
      : host(host), port(port), poller(std::forward<P>(poller)), _ac(std::forward<Args>(args)...) {}
  TcpServer(TcpServer &&) = default;
  TcpServer &operator=(TcpServer &&) = default;
  template <class Executor = coro::ExecutorBase>
  decltype(auto) accept(sys::net::SockAddr *addr) noexcept {
    return this->_ac.template accept<Executor>(addr).transform([this](auto &&res) {
      return res.transform(
          [this](auto &&skt) { return io_dev_type{std::move(skt).async(*this->poller)}; });
    });
  }

  std::string host;
  std::string port;

  std::shared_ptr<Poller> poller;

private:
  transport::Acceptor<lower_layer_type> _ac;
};
XSL_NET_NE
#endif
