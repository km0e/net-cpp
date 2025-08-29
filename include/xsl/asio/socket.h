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
#  include <xsl/compose.h>
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
struct AsyncSocketStorage {};
template <class Traits>
struct DynAsyncSocketStorage {};
XSL_ASIO_NE
XSL_NB
template <class Traits>
struct StorageUtils<_asio::AsyncSocketStorage<Traits>>
    : public Traits, public _asio::AsyncConnectionUtils<Traits> {
  using poll_traits_type = typename Traits::poll_traits_type;
};
template <class Traits>
struct StorageUtils<_asio::DynAsyncSocketStorage<Traits>>
    : public Traits,
      public _asio::AsyncConnectionUtils<Traits>,
      public _asio::AsyncDeviceUtil,
      public _asio::NetAsyncRx,
      public _asio::NetAsyncTx {
  using socket_traits_type = Traits;
  using poll_traits_type = typename Traits::poll_traits_type;
};
XSL_NE
XSL_ASIO_NB

template <class Traits>
using AsyncSocket = SharedStorageCompose<
    BaseOn<DirectAsyncReadWriteUtils>, RawOwner,
    StaticExactPubSubStorage<IOM_EVENTS, SPSCSignal2<1>, IOM_EVENTS::IN, IOM_EVENTS::OUT>,
    DynAsyncSocketStorage<Traits>>;

template <class Traits>
using DynAsyncSocket = SharedStorageCompose<
    Wrapper<AsyncReadWriteWrapper>,
    BaseOn<DirectAsyncReadWriteUtils, SharedDynamicUtil<AsyncReadWriteBase>>, RawOwner,
    StaticExactPubSubStorage<IOM_EVENTS, SPSCSignal2<1>, IOM_EVENTS::IN, IOM_EVENTS::OUT>,
    DynAsyncSocketStorage<Traits>>;

template <class Traits>
constexpr Expected<AsyncSocket<Traits>, errc> make_async_socket(Context &ctx,
                                                                sys::net::Socket<Traits> &&sock) {
  AsyncSocket<Traits> ss{};
  init_async_device(ss, std::move(sock).into_raw(), ctx);
  return {std::move(ss)};
}

template <class... Flags>
using AsyncSocketCompose = AsyncSocket<sys::net::SocketTraits<Flags...>>;

template <sys::net::ConnectionBasedSocketTraits Traits>
struct AsyncConnectionUtils<Traits> : sys::net::ConnectionUtils<Traits> {
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

inline Task<Expected<void, errc>> async_connect(int fd, const sockaddr *sa, socklen_t len,
                                                std::invocable auto &&fn) {
  auto ec = errc{};
  if (sys::filter_interrupt(::connect, fd, sa, len) != 0) {
    if (errno != EINPROGRESS) [[unlikely]] {
      ec = errc{errno};
    } else {
      CO_TRVEC(write_signal, fn());
      if (!co_await *write_signal) {
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
      int res = check(fd);
      if (res != 0) [[unlikely]] {
        // skt = std::move(async_skt).sync(poller);
        co_return std::unexpected{errc{res}};
      }
      log_debug("Connected to fd: {}", fd);
      co_return {};
    }
  }
  co_return std::unexpected{ec};
}

XSL_ASIO_NE

#endif
