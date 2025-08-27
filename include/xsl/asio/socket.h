/**
 * @file socket.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Socket type
 * @version 0.2.0
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_SOCKET
#  define XSL_ASIO_SOCKET
#  include <sys/socket.h>
#  include <xsl/asio/def.h>
#  include <xsl/asio/dev.h>
#  include <xsl/asio/io.h>
#  include <xsl/error.h>
#  include <xsl/io.h>
#  include <xsl/net.h>
#  include <xsl/sys.h>

XSL_ASIO_NB
using namespace xsl::net;
using io::IOM_EVENTS;

template <typename Traits>
struct AsyncConnectionUtils : public sys::net::ConnectionUtils<Traits> {};

template <class Traits>
class AsyncSocketUtil : public Traits,
                        public AsyncConnectionUtils<Traits>,
                        public NetAsyncRx,
                        public NetAsyncTx {
public:
  using traits_type = Traits;
  using poll_traits_type = typename traits_type::poll_traits_type;

  using value_type = byte;
};

template <class Traits>
using AsyncSocket
    = AsyncDevice<_value_pack<IOM_EVENTS::IN, IOM_EVENTS::OUT>, AsyncSocketUtil<Traits>>;

template <class Traits>
constexpr Expected<AsyncSocket<Traits>, errc> make_async_socket(Context &ctx,
                                                                sys::net::Socket<Traits> &&sock) {
  auto raw = std::move(sock).into_raw();
  TRVEC(dev, (make_async_device<IOM_EVENTS::IN, IOM_EVENTS::OUT>(ctx, std::move(raw),
                                                                 AsyncSocketUtil<Traits>{})));
  return {AsyncSocket<Traits>{std::move(dev)}};
}

template <class... Flags>
using AsyncSocketCompose = AsyncSocket<sys::net::SocketTraits<Flags...>>;

template <sys::net::ConnectionBasedSocketTraits Traits>
struct AsyncConnectionUtils<Traits> : public sys::net::ConnectionUtils<Traits> {
  using Base = sys::net::ConnectionUtils<Traits>;

  /// @brief Accept a connection
  Task<Expected<net::Socket<Traits>, errc>> accept(this auto &self,
                                                   sys::net::SockAddr<Traits> *addr = nullptr) {
    while (true) {
      auto res = Base::accept(self.raw(), addr);
      if (res) {
        co_return std::move(*res);
      } else if (res.error() == errc::resource_unavailable_try_again
                 || res.error() == errc::operation_would_block) {
        CO_ENSEC(co_await self.read_signal(), errc::not_connected);
      } else {
        co_return std::unexpected{res.error()};
      }
    }
  }
  decltype(auto) accept_async(this auto &&self, auto &ctx) noexcept {
    return self.accept().and_then(
        [&ctx](auto &&res) { return make_async_socket(ctx, std::move(res)); });
  }
};

template <class Traits>
inline Task<std::expected<AsyncSocket<Traits>, errc>> async_connect(sys::net::Socket<Traits> &skt,
                                                                    const SockAddr<Traits> &sa,
                                                                    Context &ctx) {
  auto ec = errc{};
  auto [addr, len] = sa.raw();
  if (sys::filter_interrupt(::connect, skt.raw(), &addr, len) != 0) {
    if (errno != EINPROGRESS) [[unlikely]] {
      ec = errc{errno};
    } else {
      CO_TRVEC(askt, make_async_socket(ctx, std::move(skt)));
      if (!co_await askt.write_signal()) {
        // skt = std::move(async_skt).sync(poller);
        co_return std::unexpected{errc::not_connected};
      }
      auto check = [](int fd) {
        int opt;
        socklen_t len = sizeof(opt);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &opt, &len) == -1) [[unlikely]] {
          log_error("Failed to getsockopt: {}", strerror(errno));
          return errno;
        }
        if (opt != 0) [[unlikely]] {
          log_error("Failed to connect: {}", strerror(opt));
          return opt;
        }
        return 0;
      };
      int res = check(askt.raw());
      if (res != 0) [[unlikely]] {
        // skt = std::move(async_skt).sync(poller);
        co_return std::unexpected{errc{res}};
      }
      log_debug("Connected to fd: {}", askt.raw());
      co_return std::move(askt);
    }
  }
  co_return std::unexpected{ec};
}

XSL_ASIO_NE
#endif
