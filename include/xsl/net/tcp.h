#pragma once
#ifndef XSL_NET_TCP
#  define XSL_NET_TCP
#  include "xsl/coro.h"
#  include "xsl/feature.h"
#  include "xsl/net/def.h"
#  include "xsl/sys.h"

#  include <expected>
#  include <memory>
#  include <span>
XSL_NET_NB
template <class... Flags>
Task<std::expected<sys::net::AsyncSocket<Flags...>, std::error_condition>> tcp_dial(
    const char *host, const char *port, Poller &poller) {
  auto res = sys::net::Resolver{}.resolve<Flags...>(host, port, sys::net::CLIENT_FLAGS);
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
std::expected<sys::tcp::Socket<LowerLayer>, std::error_condition> tcp_serv(const char *host,
                                                                           const char *port) {
  auto addr
      = sys::net::Resolver{}.resolve<feature::Tcp<LowerLayer>>(host, port, sys::net::SERVER_FLAGS);
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
  return sys::tcp::Socket<LowerLayer>{std::move(*bind_res)};
}

/**
 * @brief TcpServer
 *
 * @tparam LowerLayer, such as feature::Ip<Version>(Version = 4 or 6)
 */
template <class LowerLayer>
class TcpServer;

template <uint8_t Version>
class TcpServer<feature::Ip<Version>> {
public:
  using lower_layer_type = feature::Ip<Version>;
  using io_dev_type = sys::tcp::AsyncSocket<lower_layer_type>;
  using in_dev_type = io_dev_type::template rebind<feature::In>;
  using out_dev_type = io_dev_type::template rebind<feature::Out>;
  using value_type = std::unique_ptr<io_dev_type>;
  /**
   * @brief Construct a new Tcp Server object
   *
   * @param host the host to listen on
   * @param port the port to listen on
   * @param poller the poller to use
   * @return std::expected<TcpServer, std::error_condition>
   */
  static std::expected<TcpServer, std::error_condition> create(
      std::string_view host, std::string_view port, const std::shared_ptr<Poller> &poller) {
    LOG5("Start listening on {}:{}", host, port);
    auto copy_poller = poller;
    auto skt = tcp_serv<lower_layer_type>(host.data(), port.data());
    if (!skt) {
      return std::unexpected(skt.error());
    }
    return TcpServer{host, port, std::move(copy_poller), std::move(*skt).async(*copy_poller)};
  }

  template <class P, class... Args>
  TcpServer(std::string_view host, std::string_view port, P &&poller, Args &&...args)
      : host(host),
        port(port),
        poller(std::forward<P>(poller)),
        _dev(std::forward<Args>(args)...) {}
  TcpServer(TcpServer &&) = default;
  TcpServer &operator=(TcpServer &&) = default;

  /**
   * @brief read connections from the server
   *
   * @tparam Executor the executor to use
   * @param conns the connections to read
   * @return Task<Result> the result of the operation
   */
  template <class Executor = coro::ExecutorBase>
  Task<Result, Executor> read(std::span<value_type> conns) noexcept {
    std::size_t i = 0;
    for (auto &conn : conns) {
      auto res = co_await this->accept();
      if (!res) {
        co_return Result{i, res.error()};
      }
      conn = std::make_unique<io_dev_type>(std::move(*res));
      ++i;
    }
    co_return {i, std::nullopt};
  }
  Task<Result> read(std::span<value_type> conns) noexcept {
    return read<coro::ExecutorBase>(conns);
  }
  template <class Executor = coro::ExecutorBase>
  Task<std::expected<io_dev_type, std::errc>, Executor> accept() noexcept {
    while (true) {
      auto res = sys::tcp::accept(this->_dev.inner(), nullptr);
      if (res) {
        co_return std::move(*res).async(*this->poller);
      } else if (res.error() == std::errc::resource_unavailable_try_again
                 || res.error() == std::errc::operation_would_block) {
        if (!co_await this->_dev.read_sem()) {
          co_return std::unexpected{std::errc::not_connected};
        }
      } else {
        co_return std::unexpected{res.error()};
      }
    }
  }

  std::string host;
  std::string port;

  std::shared_ptr<Poller> poller;

private:
  io_dev_type _dev;
};
XSL_NET_NE
#endif
