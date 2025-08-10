/**
 * @file net.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Network utilities
 * @version 0.2.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_H
#  define XSL_ASIO_H
#  include <xsl/asio/gai.h>
#  include <xsl/asio/http.h>
#  include <xsl/asio/http/request.h>
#  include <xsl/asio/http/server.h>
#  include <xsl/asio/http/service.h>
#  include <xsl/asio/pipe.h>
#  include <xsl/asio/socket.h>
#  include <xsl/asio/tcp/server.h>

namespace xsl::asio {
  using _asio::AsyncSocket;
  using _asio::AsyncSocketCompose;
  using _asio::gai_async_connect;
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

  using _asio::get;
  using _asio::RequestPartBuilder;

  namespace http1 {
    using _asio::http::RequestLine;
    using _asio::http::Server;
    using _asio::http::ServiceBuilder;
  }  // namespace http1
  using _asio::HttpUtil;

  template <class Traits>
  struct SocketIOUtils;

  template <sys::net::SocketTraitsCompatible<sys::net::SocketTraits<TcpIp>> Traits>
  struct SocketIOUtils<Traits> : public Traits {
    using io_dev_type = AsyncSocket<Traits>;

    template <class Poller>
    Expected<tcp::Server<Traits>> make_creator(
        const std::shared_ptr<Poller> &poller, std::string_view host,
        std::string_view port)  /// TODO: add attr opt for socket
    {
      log_debug("Start listening on {}:{}", host, port);
      auto copy_poller = poller;
      auto skt = net::gai_bind<Traits>(host.data(), port.data());
      if (!skt) return std::unexpected(skt.error());
      HNSURE(skt->listen());
      return {{host, port, std::move(copy_poller), *poller, std::move(*skt)}};
    }
  };

  template <sys::net::SocketTraitsCompatible<sys::net::SocketTraits<UdpIp>> Traits>
  struct SocketIOUtils<Traits> : public Traits {
    using io_dev_type = AsyncSocket<Traits>;

    template <class Poller>
    Expected<AsyncSocket<Traits>> make_io(Poller &poller, std::string_view host,
                                          std::string_view port)  /// TODO: add attr opt for socket
    {
      auto skt = net::gai_bind<Traits>(host.data(), port.data());
      if (!skt) return std::unexpected(skt.error());
      return {{poller, std::move(*skt)}};
    }
    template <class Poller>
    Expected<AsyncSocket<Traits>> make_io_to(
        Poller &poller, std::string_view host,
        std::string_view port)  /// TODO: add attr opt for socket
    {
      auto skt = net::gai_connect<Traits>(host.data(), port.data());
      if (!skt) return std::unexpected(skt.error());
      return {{poller, std::move(*skt)}};
    }
  };
  template <class... Flags>
  consteval auto make_socket_io_utils() {
    return SocketIOUtils<_sys::net::SocketTraits<Flags...>>();
  }

}  // namespace xsl::asio
#endif
