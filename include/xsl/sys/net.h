/**
 * @file net.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Sys Net utilities
 * @version 0.1.0
 * @date 2025-08-29
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_SYS_NET
#  define XSL_SYS_NET
#  include <xsl/sys/def.h>
#  include <xsl/sys/net/def.h>
#  include <xsl/sys/net/socket.h>
#  include <xsl/sys/net/utils.h>
XSL_SYS_NB
namespace inet = net::inet;
namespace detail {
  using net::SocketCompose, net::SocketTraits;
  template <class Traits>
  struct TcpIpSocketUtils : Traits {
    using io_dev_type = net::Socket<Traits>;

    template <class... Flags>  /// TODO: add attr opt for socket
    Expected<SocketCompose<Traits, Flags...>> c(const char *ip, inet::port_t port) noexcept {
      using traits_type = SocketTraits<Traits, Flags...>;
      TRV(addr, net::make_sockaddr<traits_type>(ip, port));
      TRV(sock, socket(addr));
      ENSURE(sock.reuse_addr());
      ENSURE(sock.bind(addr));
      return std::move(sock);
    }
  };

  template <class Traits>
  struct UdpIpSocketUtils : Traits {
    using io_dev_type = net::Socket<Traits>;

    template <class... Flags>  /// TODO: add attr opt for socket
    Expected<SocketCompose<Traits, Flags...>> c(const char *ip, inet::port_t port) noexcept {
      using traits_type = SocketTraits<Traits, Flags...>;
      TRV(addr, net::make_sockaddr<traits_type>(ip, port));
      TRV(sock, socket(addr));
      ENSURE(sock.reuse_addr());
      ENSURE(sock.bind(addr));
      return std::move(sock);
    }
    template <class... Flags>  /// TODO: add attr opt for socket
    Expected<SocketCompose<Traits, Flags...>> c2(const char *ip, inet::port_t port) noexcept {
      using traits_type = SocketTraits<Traits, Flags...>;
      TRV(addr, net::make_sockaddr<traits_type>(ip, port));
      TRV(sock, socket(addr));
      ENSURE(sock.connect(addr));
      return std::move(sock);
    }
  };
}  // namespace detail

template <class Traits>
struct SocketUtils {
  using io_dev_type = net::Socket<Traits>;
  template <class... Flags>  /// TODO: add attr opt for socket
  Expected<net::SocketCompose<Traits, Flags...>> c(const char *ip, inet::port_t port) noexcept;
};

template <>
struct SocketUtils<net::TcpIpv4SocketTraits> : detail::TcpIpSocketUtils<net::TcpIpv4SocketTraits> {
};

template <>
struct SocketUtils<net::TcpIpSocketTraits> : detail::TcpIpSocketUtils<net::TcpIpSocketTraits> {};

template <>
struct SocketUtils<net::TcpIpv6SocketTraits> : detail::TcpIpSocketUtils<net::TcpIpv6SocketTraits> {
};

template <>
struct SocketUtils<net::UdpIpv4SocketTraits> : detail::UdpIpSocketUtils<net::UdpIpv4SocketTraits> {
};

template <>
struct SocketUtils<net::UdpIpSocketTraits> : detail::UdpIpSocketUtils<net::UdpIpSocketTraits> {};

template <class... Flags>
consteval auto make_socket_utils() {
  return SocketUtils<net::SocketTraits<Flags...>>();
}
XSL_SYS_NE
#endif
