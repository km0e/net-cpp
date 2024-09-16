/**
 * @file server.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief TcpServer
 * @version 0.12
 * @date 2024-08-18
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#include "xsl/sys/net/gai.h"
#ifndef XSL_NET_TCP_SERVER
#  define XSL_NET_TCP_SERVER
#  include "xsl/feature.h"
#  include "xsl/net/tcp/def.h"
#  include "xsl/sys.h"

#  include <cstdint>
#  include <expected>
#  include <string_view>
#  include <system_error>
XSL_TCP_NB
using namespace xsl::io;

/**
 * @brief TcpServer
 *
 * @tparam LowerLayer, such as Ip<Version>(Version = 4 or 6)
 */
template <class LowerLayer>
class Server;
template <std::uint8_t Version>
class Server<Ip<Version>> {
public:
  using lower_layer_type = Ip<Version>;
  using io_dev_type = sys::tcp::AsyncSocket<lower_layer_type>;
  using io_traits_type = AIOTraits<io_dev_type>;
  using value_type = std::unique_ptr<io_dev_type>;

  constexpr Server(std::string_view host, std::string_view port, auto &&poller, auto &&...args)
      : host(host),
        port(port),
        poller(std::forward<decltype(poller)>(poller)),
        _dev(std::forward<decltype(args)>(args)...) {}
  constexpr Server(Server &&) = default;
  constexpr Server &operator=(Server &&) = default;

  /**
   * @brief read data from the device
   *
   * @param conns the connections
   * @return Task<Result>
   */
  Task<Result> read(std::span<value_type> conns) noexcept {
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
  /// @brief accept a connection
  constexpr decltype(auto) accept() noexcept {
    return this->_dev.accept().then([this](auto &&res) {
      return res.transform([this](auto &&skt) { return std::move(skt).async(*this->poller); });
    });
  }

  std::string host;
  std::string port;

  std::shared_ptr<Poller> poller;

private:
  io_dev_type _dev;
};
/**
 * @brief make a server
 *
 * @tparam LowerLayer the lower layer type, such as Ip<Version>(Version = 4 or 6)
 * @param host the host
 * @param port the port
 * @param poller the poller
 * @return std::expected<Server<LowerLayer>, std::error_condition>
 */
template <class LowerLayer>
constexpr std::expected<Server<LowerLayer>, std::error_condition> make_server(
    std::string_view host, std::string_view port, const std::shared_ptr<Poller> &poller) {
  LOG5("Start listening on {}:{}", host, port);
  auto copy_poller = poller;
  auto skt = _sys::net::gai_bind<Tcp<LowerLayer>>(host.data(), port.data());
  if (!skt) {
    return std::unexpected(skt.error());
  }
  auto ec = skt->listen();
  if (ec != errc{}) {
    return std::unexpected(ec);
  }
  auto async_skt = std::move(*skt).async(*copy_poller);
  return Server<LowerLayer>{host, port, std::move(copy_poller), std::move(async_skt)};
}
XSL_TCP_NE
#endif
