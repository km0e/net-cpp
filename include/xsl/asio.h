/**
 * @file net.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Network utilities
 * @version 0.2.2
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_H
#  define XSL_ASIO_H
#  include <xsl/asio/http.h>
#  include <xsl/asio/http/request.h>
#  include <xsl/asio/http/server.h>
#  include <xsl/asio/http/service.h>
#  include <xsl/asio/pipe.h>
#  include <xsl/asio/socket.h>
#  include <xsl/asio/tcp/server.h>
#  include <xsl/error.h>
#  include <xsl/feature.h>
#  include <xsl/sys.h>

namespace xsl::asio {
  using _asio::AsyncSocket;
  using _asio::AsyncSocketCompose;
  using _asio::DynAsyncSocket;
  using _asio::HttpClient;
  using _asio::make_async_socket;
  using _asio::splice_bidirectional;
  using _asio::TLSContextBuilder;
  using _asio::TLSMode;
  using sys::net::gai_connect;
  using sys::net::getaddrinfo;
  using sys::net::SockAddr;
  using sys::net::SockAddrCompose;
  using sys::net::Socket;
  using sys::net::SocketCompose;
  //

  using _asio::Buffer;
  namespace tcp {
    using _asio::tcp::Server;
  }  // namespace tcp

  namespace udp {}  // namespace udp

  using _asio::http::HandleContext;
  using _asio::http::HandleResult;
  using _asio::http::Method;
  using _asio::http::Request;
  using _asio::http::RequestLine;
  using _asio::http::ResponseBuilder;
  using _asio::http::ResponsePart;
  using _asio::http::RouteContext;
  using _asio::http::Router;
  using _asio::http::StaticFileConfig;
  using _asio::http::Status;
  using _asio::http::to_string_view;

  using _asio::RequestPartBuilder;

  namespace http1 {
    using _asio::http::RequestLine;
    using _asio::http::Server;
    using _asio::http::ServiceBuilder;
  }  // namespace http1
  using _asio::HttpUtil;

  template <class Traits>
  struct AsyncSocketUtils;

  template <sys::net::SocketTraitsCompatible<sys::net::SocketTraits<TcpIp>> Traits>
  struct AsyncSocketUtils<Traits> : public Traits {
    using io_dev_type = AsyncSocket<Traits>;

    /// TODO: add attr opt for socket
    template <class Ctx>
    Expected<tcp::Server<Traits>> c_creator(const std::shared_ptr<Ctx>& ctx, std::string_view ip,
                                            sys::net::inet::port_t port) noexcept {
      TRV(sock, sock_utils.c(ip.data(), port));
      ENSURE(sock.listen());
      auto copy_ctx = ctx;
      TRV(asock, make_async_socket(*copy_ctx, std::move(sock)));
      return {tcp::Server<Traits>{std::string{ip}, port, std::move(copy_ctx), std::move(asock)}};
    }
    template <class Ctx>
    Expected<tcp::Server<Traits>> c_creator(const std::shared_ptr<Ctx>& ctx, const char* ip,
                                            const char* port) noexcept {
      return c_creator(ctx, ip, static_cast<sys::net::inet::port_t>(std::atoi(port)));
    }
    template <class Ctx>
    decltype(auto) c_creator(std::shared_ptr<Ctx>& ctx, std::string_view ip,
                             const char* port) noexcept {
      return c_creator(ctx, ip, static_cast<sys::net::inet::port_t>(std::atoi(port)));
    }
    template <class Ctx>
    decltype(auto) c_creator(std::shared_ptr<Ctx>& ctx, std::string_view ip,
                             std::string_view port) noexcept {
      return c_creator(ctx, ip, static_cast<sys::net::inet::port_t>(std::atoi(port.data())));
    }
    template <class... Flags>
    Expected<AsyncSocketCompose<Traits, Flags...>> c(auto& ctx, const char* ip,
                                                     sys::net::inet::port_t port) noexcept {
      TRV(sock, sock_utils.template c<Flags...>(ip, port));
      TRV(asock, make_async_socket(ctx, std::move(sock)));
      return std::move(asock);
    }

    template <class... Flags>
    Task<Expected<AsyncSocketCompose<Traits, Flags...>>> ac2(auto& ctx, const char* ip,
                                                             sys::net::inet::port_t port) noexcept {
      using traits_type = sys::net::SocketTraits<Traits, Flags...>;
      CO_TRV(addr, sys::net::make_sockaddr<traits_type>(ip, port));
      AsyncSocket<traits_type> asock;
      CO_TRV(sock, sys::net::socket(addr));
      CO_ENSURE(
          co_await _asio::async_connect(sock.raw(), &addr.addr(), addr.len(),
                                        [&] -> Expected<decltype(&asock.write_signal()), errc> {
                                          TRVEC(tasock, make_async_socket(ctx, std::move(sock)));
                                          asock = std::move(tasock);
                                          return {&asock.write_signal()};
                                        }));
      co_return std::move(asock);
    }
    template <class... Flags>
    decltype(auto) ac2(auto& ctx, const char* ip, const char* port) noexcept {
      return ac2<Flags...>(ctx, ip, static_cast<sys::net::inet::port_t>(std::atoi(port)));
    }
    template <class... Flags, class _Traits = sys::net::SocketTraits<Traits, Flags...>>
    Task<Expected<AsyncSocket<_Traits>, errc>> ac2(auto& ctx,
                                                   sys::net::AddrInfos<Traits>& ais) noexcept {
      Expected<AsyncSocket<_Traits>, errc> asock;
      for (addrinfo& ai : ais) {
        CONTV(sock, sys::net::socket<_Traits>(ai));
        auto res = co_await _asio::async_connect(
            sock.raw(), ai.ai_addr, ai.ai_addrlen,
            [&] -> Expected<decltype(&asock->write_signal()), errc> {
              TRVEC(tasock, make_async_socket(ctx, std::move(sock)));
              asock = std::move(tasock);
              return {&asock->write_signal()};
            });
        if (res) {
          break;
        }
        asock = std::unexpected(res.error());
      }
      co_return asock;
    }
    template <class... Flags, class _Traits = sys::net::SocketTraits<Traits, Flags...>>
    Task<Expected<AsyncSocket<_Traits>, errc>> ac2(auto& ctx,
                                                   sys::net::AddrInfos<Traits>&& ais) noexcept {
      return ac2_impl<AsyncSocket<_Traits>, _Traits>(ctx, ais);
    }

    template <class... Flags, class _Traits = sys::net::SocketTraits<Traits, Flags...>>
    Task<Expected<DynAsyncSocket<_Traits>, errc>> ac2_dyn(
        auto& ctx, sys::net::AddrInfos<Traits>& ais) noexcept {
      return ac2_impl<DynAsyncSocket<_Traits>, _Traits>(ctx, ais);
    }

    sys::SocketUtils<Traits> sock_utils;

  private:
    template <class AsyncSocket, class _Traits>
    Task<Expected<AsyncSocket, errc>> ac2_impl(auto& ctx,
                                               sys::net::AddrInfos<Traits>& ais) noexcept {
      Expected<AsyncSocket, errc> asock;
      for (addrinfo& ai : ais) {
        CONTV(sock, sys::net::socket<_Traits>(ai));
        auto res = co_await _asio::async_connect(
            sock.raw(), ai.ai_addr, ai.ai_addrlen,
            [&] -> Expected<decltype(&(*asock)->write_signal()), errc> {
              AsyncSocket ss{};
              _asio::init_async_device(ss, std::move(sock).into_raw(), ctx);
              asock = std::move(ss);
              return {&(*asock)->write_signal()};
            });
        if (res) {
          break;
        }
        asock = std::unexpected(res.error());
      }
      co_return asock;
    }
  };

  template <sys::net::SocketTraitsCompatible<sys::net::SocketTraits<UdpIp>> Traits>
  struct AsyncSocketUtils<Traits> : public Traits {
    using io_dev_type = AsyncSocket<Traits>;
    /// TODO: add attr opt for socket
    Expected<AsyncSocket<Traits>> c(auto& ctx, std::string_view ip, sys::net::inet::port_t port) {
      TRV(sock, sock_utils.c(ip.data(), port));
      TRV(asock, make_async_socket(ctx, std::move(sock)));
      return {std::move(asock)};
    }
    decltype(auto) c(auto& ctx, std::string_view ip, std::string_view port) {
      return c(ctx, ip, static_cast<sys::net::inet::port_t>(std::atoi(port.data())));
    }
    Expected<AsyncSocket<Traits>> c2(auto& ctx, std::string_view ip, sys::net::inet::port_t port) {
      TRV(sock, sock_utils.c2(ip.data(), port));
      TRV(asock, make_async_socket(ctx, std::move(sock)));
      return {std::move(asock)};
    }
    decltype(auto) c2(auto& ctx, std::string_view ip, std::string_view port) {
      return c2(ctx, ip, static_cast<sys::net::inet::port_t>(std::atoi(port.data())));
    }
    sys::SocketUtils<Traits> sock_utils [[no_unique_address]];
  };

  template <class... Flags>
  consteval auto make_async_socket_utils() {
    return AsyncSocketUtils<sys::net::SocketTraits<Flags...>>();
  }
}  // namespace xsl::asio
#endif
