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
#ifndef XSL_ASIO_TCP_SERVER
#  define XSL_ASIO_TCP_SERVER
#  include "xsl/asio/socket.h"
#  include "xsl/asio/tcp/def.h"
#  include "xsl/feature.h"
#  include "xsl/io/def.h"
#  include "xsl/sys.h"

#  include <cstdint>
#  include <expected>
#  include <string_view>
#  include <system_error>
XSL_ASIO_TCP_NB
using namespace xsl::io;
using namespace xsl::net;

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
  using io_dev_type = AsyncSocket<sys::net::SocketTraits<Tcp<lower_layer_type>>>;
  using value_type = io_dev_type;

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
  Task<io::Result> read(std::span<io_dev_type> conns) noexcept {
    std::size_t i = 0;
    for (auto &conn : conns) {
      auto res = co_await this->accept();
      if (!res) {
        co_return io::Result{i, res.error()};
      }
      conn = AsyncSocket(std::move(*res), *this->poller);
      ++i;
    }
    co_return {i, std::nullopt};
  }
  /// @brief accept a connection
  constexpr decltype(auto) accept() noexcept {
    return this->_dev.accept().then([this](auto &&res) {
      return res.transform(
          [this](auto &&skt) { return AsyncSocket(std::move(skt), *this->poller); });
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
  log_debug("Start listening on {}:{}", host, port);
  auto copy_poller = poller;
  auto skt = gai_bind<Tcp<LowerLayer>>(host.data(), port.data());
  if (!skt) {
    return std::unexpected(skt.error());
  }
  auto ec = skt->listen();
  if (ec != errc{}) {
    return std::unexpected(ec);
  }
  auto async_skt = AsyncSocket(std::move(*skt), *copy_poller);
  return Server<LowerLayer>{host, port, std::move(copy_poller), std::move(async_skt)};
}
XSL_ASIO_TCP_NE
#endif
