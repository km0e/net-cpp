/**
 * @file server.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief TcpServer
 * @version 0.1.4
 * @date 2024-08-18
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_TCP_SERVER
#  define XSL_ASIO_TCP_SERVER
#  include <xsl/asio/net/socket.h>
#  include <xsl/asio/net/tcp/def.h>
#  include <xsl/io.h>

XSL_ASIO_TCP_NB
using namespace xsl::io;
using namespace xsl::net;

/**
 * @brief TcpServer
 *
 * @tparam LowerLayer, such as Ip<Version>(Version = 4 or 6)
 */
template <sys::net::SocketTraitsCompatible<sys::net::SocketTraits<TcpIp>> Traits>
class Server {
public:
  using io_dev_type = AsyncSocket<Traits>;

  constexpr Server(std::string&& host, sys::net::inet::port_t port, auto&& ctx, auto&&... args)
      : host(host),
        port(port),
        ctx(std::forward<decltype(ctx)>(ctx)),
        _dev(std::forward<decltype(args)>(args)...) {}
  constexpr Server(Server&&) = default;
  constexpr Server& operator=(Server&&) = default;

  /**
   * @brief read data from the device
   *
   * @param conns the connections
   * @return Task<Result>
   */
  Task<io::Result> read(std::span<io_dev_type> conns) noexcept {
    auto i = 0uz;
    for (auto& conn : conns) {
      auto res = co_await this->accept();
      if (!res) {
        co_return io::Result{i, res.error()};
      }
      conn = io_dev_type(*this->ctx, std::move(*res));
      ++i;
    }
    co_return {i, std::nullopt};
  }
  /// @brief accept a connection
  constexpr decltype(auto) accept() noexcept { return this->_dev.accept(); }
  /// @brief accept a connection with async
  constexpr decltype(auto) accept_async() noexcept { return this->_dev->accept_async(*ctx); }
  std::string host;
  sys::net::inet::port_t port;

  std::shared_ptr<IOContext> ctx;

private:
  io_dev_type _dev;
};

XSL_ASIO_TCP_NE
#endif
