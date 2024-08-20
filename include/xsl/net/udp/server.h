/**
 * @file server.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-19
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_UDP_SERVER
#  define XSL_NET_UDP_SERVER
#  include "xsl/feature.h"
#  include "xsl/net/udp/def.h"
#  include "xsl/net/udp/utils.h"
#  include "xsl/sys.h"

#  include <cstdint>
#  include <expected>
#  include <string_view>
#  include <system_error>
XSL_UDP_NB
/**
 * @brief TcpServer
 *
 * @tparam LowerLayer, such as feature::Ip<Version>(Version = 4 or 6)
 */
template <class LowerLayer>
class Server;
template <std::uint8_t Version>
class Server<feature::Ip<Version>> {
public:
  using lower_layer_type = feature::Ip<Version>;
  using io_dev_type = sys::tcp::AsyncSocket<lower_layer_type>;
  using in_dev_type = io_dev_type::template rebind<feature::In>;
  using out_dev_type = io_dev_type::template rebind<feature::Out>;
  using value_type = std::unique_ptr<io_dev_type>;

  template <class P, class... Args>
  Server(std::string_view host, std::string_view port, P &&poller, Args &&...args)
      : host(host),
        port(port),
        poller(std::forward<P>(poller)),
        _dev(std::forward<Args>(args)...) {}
  Server(Server &&) = default;
  Server &operator=(Server &&) = default;

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
      auto res = sys::tcp::accept(this->_dev, nullptr);
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
template <class LowerLayer>
std::expected<Server<LowerLayer>, std::error_condition> make_server(
    std::string_view host, std::string_view port, const std::shared_ptr<Poller> &poller) {
  LOG5("Start listening on {}:{}", host, port);
  auto copy_poller = poller;
  auto skt = serv<LowerLayer>(host.data(), port.data());
  if (!skt) {
    return std::unexpected(skt.error());
  }
  return Server<LowerLayer>{host, port, std::move(copy_poller),
                            std::move(*skt).async(*copy_poller)};
}
XSL_UDP_NE
#endif
