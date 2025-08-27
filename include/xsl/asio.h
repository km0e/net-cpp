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
  using _asio::make_async_socket;
  using _asio::splice_bidirectional;
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
    Expected<tcp::Server<Traits>> c_creator(std::shared_ptr<Ctx>& ctx, std::string_view ip,
                                            sys::net::inet::port_t port) noexcept {
      TRV(sock, sock_utils.c(ip.data(), port));
      ENSURE(sock.listen());
      TRV(asock, make_async_socket(*ctx, std::move(sock)));
      auto copy_ctx = ctx;
      return {tcp::Server<Traits>{std::string{ip}, port, std::move(copy_ctx), std::move(asock)}};
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
      CO_TRV(sock, socket(addr));
      CO_ENSURE(co_await _asio::async_connect(sock, addr, ctx));
      CO_TRV(asock, make_async_socket(ctx, std::move(sock)));
      co_return std::move(asock);
    }
    template <class... Flags>
    decltype(auto) ac2(auto& ctx, const char* ip, const char* port) noexcept {
      return ac2<Flags...>(ctx, ip, static_cast<sys::net::inet::port_t>(std::atoi(port)));
    }

    sys::net::SocketUtils<Traits> sock_utils;
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
    sys::net::SocketUtils<Traits> sock_utils [[no_unique_address]];
  };

  template <class... Flags>
  consteval auto make_async_socket_utils() {
    return AsyncSocketUtils<sys::net::SocketTraits<Flags...>>();
  }
}  // namespace xsl::asio
#endif
