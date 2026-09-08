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

using sys::IOContext;
using sys::IOM_EVENTS;

using xsl::net::SockAddr;

template <class Traits>
struct AsyncConnectionUtils : public sys::net::ConnectionUtils<Traits> {
  using socket_traits_type = Traits;
};

namespace _detail {
  template <class Traits>
  using AsyncSocketInner
      = DefaultEpollWrapper<LocalCompose<RawOwner, IOSignalStorage<IOM_EVENTS::IN, IOM_EVENTS::OUT>,
                                         asio::AsyncConnectionUtils<Traits>, asio::AsyncDeviceUtil,
                                         asio::NetAsyncRx, asio::NetAsyncTx>>;
};  // namespace _detail

template <class Traits>
using AsyncSocket = DirectAsyncReadWriteWrapper<shared_memory<_detail::AsyncSocketInner<Traits>>>;

template <class Traits>
using DynAsyncSocket = DirectAsyncReadWriteWrapper<
    shared_memory<AsyncReadWriteWrapper<_detail::AsyncSocketInner<Traits>>>>;

template <class Traits>
constexpr Expected<AsyncSocket<Traits>, errc> make_async_socket(IOContext& ctx,
                                                                sys::net::Socket<Traits>&& sock);

template <sys::net::ConnectionBasedSocketTraits Traits>
struct AsyncConnectionUtils<Traits> : public sys::net::ConnectionUtils<Traits> {
  using Base = sys::net::ConnectionUtils<Traits>;

  /// @brief Accept a connection
  /// @note the loop stops when the device reports that its IOContext has been
  ///       shut down, otherwise a dead poller would make this loop hang forever
  Task<Expected<net::Socket<Traits>, errc>> accept(this auto& self,
                                                   sys::net::SockAddr<Traits>* addr = nullptr) {
    while (true) {
      auto res = Base::accept(self.raw(), addr);
      if (res) {
        // low-latency default for accepted connections (see no_delay notes)
        if constexpr (requires { res->no_delay(); }) {
          (void)res->no_delay();
        }
        co_return std::move(*res);
      } else if (res.error() == errc::resource_unavailable_try_again
                 || res.error() == errc::operation_would_block) {
        if constexpr (requires { self.stopped(); }) {
          if (self.stopped()) {
            co_return std::unexpected{errc::operation_canceled};
          }
        }
        CO_ENSEC(co_await self.read_signal());
      } else {
        co_return std::unexpected{res.error()};
      }
    }
  }
  decltype(auto) accept_async(this auto&& self, auto& ctx) noexcept {
    return self.accept().and_then(
        [&ctx](auto&& res) { return make_async_socket(ctx, std::move(res)); });
  }
};

template <class Traits>
constexpr Expected<AsyncSocket<Traits>, errc> make_async_socket(IOContext& ctx,
                                                                sys::net::Socket<Traits>&& sock) {
  AsyncSocket<Traits> ss{};
  ENSEC(init_async_device(ss, std::move(sock).into_raw(), ctx));
  return {std::move(ss)};
}

template <class... Flags>
using AsyncSocketCompose = AsyncSocket<sys::net::SocketTraits<Flags...>>;

Task<Expected<void, errc>> async_connect(auto& skt, RawOwner&& o, const sockaddr* sa,
                                         socklen_t len) {
  auto ec = errc{};
  auto fd = o.raw();
  if (sys::filter_interrupt(::connect, fd, sa, len) != 0) {
    if (errno != EINPROGRESS) [[unlikely]] {
      ec = errc{errno};
    } else {
      log_debug("Connecting to fd: {}", fd);
      CO_ENSEC(init_async_device(skt, std::move(o), co_await CurrentIOContext));
      if (!co_await skt->write_signal()) {
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
        co_return std::unexpected{errc{res}};
      }
      log_debug("Connected to fd: {}", fd);
      co_return {};
    }
  }
  co_return std::unexpected{ec};
}

template <class Traits>
struct AsyncSocketCreator;

template <sys::net::SocketTraitsCompatible<sys::net::SocketTraits<TcpIp>> Traits>
struct AsyncSocketCreator<Traits> : public Traits {
  using io_dev_type = AsyncSocket<Traits>;

  template <class... Flags>
  Expected<AsyncSocketCompose<Traits, Flags...>, errc> cb(IOContext& ctx, const char* ip,
                                                          sys::net::inet::port_t port) noexcept {
    TRVEC(sock, sock_utils.template cb<Flags...>(ip, port));
    // listen BEFORE registering with the poller: a bound-but-not-listening
    // TCP socket reports EPOLLOUT|EPOLLHUP in epoll, and the handler treats
    // HUP as DELETE — deregistering the device before it ever served. A later
    // explicit listen() is legal and only updates the backlog.
    ENSEC(sock.listen());
    TRVEC(asock, make_async_socket(ctx, std::move(sock)));
    return std::move(asock);
  }
  template <class... Flags, class... Args>
  Expected<AsyncSocketCompose<Traits, Flags...>, errc> cl(IOContext& ctx, Args&&... args) noexcept {
    TRVEC(sock, sock_utils.template cl<Flags...>(std::forward<Args>(args)...));
    TRVEC(asock, make_async_socket(ctx, std::move(sock)));
    return std::move(asock);
  }
  template <class AsyncSocket, sys::net::SocketTraitsCompatible<Traits> _Traits>
  Task<Expected<void, errc>> a2(AsyncSocket& asock, const SockAddr<_Traits>& addr) noexcept {
    CO_TRVEC(sock, sys::net::socket(addr));
    CO_ENSEC(
        co_await asio::async_connect(asock, std::move(sock).into_raw(), &addr.addr(), addr.len()));
  }
  template <sys::net::SocketTraitsCompatible<Traits> _Traits>
  decltype(auto) ca2(const SockAddr<_Traits>& addr) noexcept {
    AsyncSocket<_Traits> asock;
    CO_TRVEC(sock, sys::net::socket(addr));
    CO_ENSEC(
        co_await asio::async_connect(asock, std::move(sock).into_raw(), &addr.addr(), addr.len()));
    co_return std::move(asock);
  }
  template <class... Flags, class... Args, class _Traits = sys::net::SocketTraits<Traits, Flags...>>
    requires requires(Args&&... args) {
      sys::net::make_sockaddr<_Traits>(std::forward<Args>(args)...);
    }
  Task<Expected<AsyncSocket<_Traits>, errc>> ca2(Args&&... args) noexcept {
    AsyncSocket<_Traits> asock;
    CO_TRVEC(sa, sys::net::make_sockaddr<_Traits>(std::forward<Args>(args)...));
    CO_TRVEC(sock, sys::net::socket(sa));
    CO_ENSEC(co_await asio::async_connect(asock, std::move(sock).into_raw(), &sa.addr(), sa.len()));
    co_return std::move(asock);
  }

  template <class AsyncSocket, sys::net::SocketTraitsCompatible<Traits> _Traits>
  decltype(auto) a2_dyn(AsyncSocket& asock, sys::net::AddrInfos<_Traits>& ais) noexcept {
    return ca2_impl<AsyncSocket, _Traits>(asock, ais);
  }
  template <sys::net::SocketTraitsCompatible<Traits> _Traits>
  Task<Expected<DynAsyncSocket<_Traits>, errc>> ca2_dyn(
      sys::net::AddrInfos<_Traits>& ais) noexcept {
    DynAsyncSocket<_Traits> asock;
    CO_ENSEC(co_await a2_dyn(asock, ais));
    co_return std::move(asock);
  }

  sys::SocketCreator<Traits> sock_utils;

private:
  template <class AsyncSocket, class _Traits>
  Task<Expected<void, errc>> ca2_impl(AsyncSocket& asock,
                                      sys::net::AddrInfos<Traits>& ais) noexcept {
    errc ec = {};
    for (addrinfo& ai : ais) {
      CONTV(sock, sys::net::socket<_Traits>(ai));
      CO_TRVEC(addr, sys::net::inet_n2p(reinterpret_cast<sockaddr_storage&>(*ai.ai_addr)));
      // addr is only consumed by log_debug, which compiles out at higher log levels
      (void)addr;
      log_debug("Trying to connect to {}", addr);
      auto res = co_await asio::async_connect(asock, std::move(sock).into_raw(), ai.ai_addr,
                                              ai.ai_addrlen);
      if (res) {
        ec = errc{};
        break;
      }
      ec = res.error();
    }
    CO_ENSEC(ec);
  }
};

template <sys::net::SocketTraitsCompatible<sys::net::SocketTraits<UdpIp>> Traits>
struct AsyncSocketCreator<Traits> : public Traits {
  using io_dev_type = AsyncSocket<Traits>;
  /// TODO: add attr opt for socket
  Expected<AsyncSocket<Traits>> c(IOContext& ctx, std::string_view ip,
                                  sys::net::inet::port_t port) {
    TRV(sock, sock_utils.c(ip.data(), port));
    TRV(asock, make_async_socket(ctx, std::move(sock)));
    return {std::move(asock)};
  }
  decltype(auto) c(auto& ctx, std::string_view ip, std::string_view port) {
    return c(ctx, ip, static_cast<sys::net::inet::port_t>(std::atoi(port.data())));
  }
  Expected<AsyncSocket<Traits>> c2(IOContext& ctx, std::string_view ip,
                                   sys::net::inet::port_t port) {
    TRV(sock, sock_utils.c2(ip.data(), port));
    TRV(asock, make_async_socket(ctx, std::move(sock)));
    return {std::move(asock)};
  }
  decltype(auto) c2(IOContext& ctx, std::string_view ip, std::string_view port) {
    return c2(ctx, ip, static_cast<sys::net::inet::port_t>(std::atoi(port.data())));
  }
  sys::SocketCreator<Traits> sock_utils [[no_unique_address]];
};

template <class... Flags>
using AsyncSocketCreatorCompose = AsyncSocketCreator<sys::net::SocketTraits<Flags...>>;

XSL_ASIO_NE

#endif
