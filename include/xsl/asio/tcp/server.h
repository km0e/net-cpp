/**
 * @file server.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief TcpServer
 * @version 0.1.3
 * @date 2024-08-18
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_TCP_SERVER
#  define XSL_ASIO_TCP_SERVER
#  include <xsl/asio/socket.h>
#  include <xsl/asio/tcp/def.h>
#  include <xsl/io.h>

#  include <string_view>
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

  constexpr Server(std::string_view host, std::string_view port, auto &&ctx, auto &&...args)
      : host(host),
        port(port),
        ctx(std::forward<decltype(ctx)>(ctx)),
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
      conn = io_dev_type(*this->ctx, std::move(*res));
      ++i;
    }
    co_return {i, std::nullopt};
  }
  /// @brief accept a connection
  constexpr decltype(auto) accept() noexcept {
    return this->_dev.accept().then([this](auto &&res) {
      return res.transform([this](auto &&skt) { return io_dev_type(*this->ctx, std::move(skt)); });
    });
  }
  std::string host;
  std::string port;

  std::shared_ptr<Context> ctx;

private:
  io_dev_type _dev;
};

XSL_ASIO_TCP_NE
#endif
