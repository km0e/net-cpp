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
namespace _detail {
  using net::SocketCompose, net::SocketTraits;
  template <class Traits>
  struct TcpIpSocketCreator : Traits {
    using io_dev_type = net::Socket<Traits>;

    template <net::SocketTraitsCompatible<Traits> _Traits>  /// TODO: add attr opt for socket
    Expected<void, errc> b(net::Socket<_Traits>& sock,
                           const net::SockAddr<_Traits>& addr) noexcept {
      ENSEC(sock.reuse_addr());
      ENSEC(sock.bind(addr));
      return {};
    }
    template <net::SocketTraitsCompatible<Traits> _Traits>  /// TODO: add attr opt for socket
    Expected<net::Socket<_Traits>, errc> cb(const net::SockAddr<_Traits>& addr) noexcept {
      TRVEC(sock, net::socket(addr));
      ENSEC(b(sock, addr));
      return std::move(sock);
    }
    template <class... Flags, class... Args>
      requires requires(Args&&... args) {
        net::make_sockaddr<SocketTraits<Traits, Flags...>>(std::forward<Args>(args)...);
      }
    /// TODO: add attr opt for socket
    Expected<net::Socket<SocketTraits<Traits, Flags...>>, errc> cb(Args&&... args) noexcept {
      using traits_type = SocketTraits<Traits, Flags...>;
      TRVEC(addr, net::make_sockaddr<traits_type>(std::forward<Args>(args)...));
      return cb(addr);
    }
    template <net::SocketTraitsCompatible<Traits> _Traits>  /// TODO: add attr opt for socket
    Expected<void, errc> l(net::Socket<_Traits>& sock,
                           const net::SockAddr<_Traits>& addr) noexcept {
      ENSEC(b(sock, addr));
      ENSEC(sock.listen());
      return {};
    }
    template <net::SocketTraitsCompatible<Traits> _Traits>  /// TODO: add attr opt for socket
    Expected<net::Socket<_Traits>, errc> cl(const net::SockAddr<_Traits>& addr) noexcept {
      TRVEC(sock, net::socket(addr));
      ENSEC(l(sock, addr));
      return std::move(sock);
    }
    template <class... Flags, class... Args>
      requires requires(Args&&... args) {
        net::make_sockaddr<SocketTraits<Traits, Flags...>>(std::forward<Args>(args)...);
      }
    Expected<net::Socket<SocketTraits<Traits, Flags...>>, errc> cl(Args&&... args) noexcept {
      using traits_type = SocketTraits<Traits, Flags...>;
      TRVEC(addr, net::make_sockaddr<traits_type>(std::forward<Args>(args)...));
      return cl(addr);
    }
  };

  template <class Traits>
  struct UdpIpSocketCreator : Traits {
    using io_dev_type = net::Socket<Traits>;

    template <class... Flags>  /// TODO: add attr opt for socket
    Expected<SocketCompose<Traits, Flags...>> c(const char* ip, inet::port_t port) noexcept {
      using traits_type = SocketTraits<Traits, Flags...>;
      TRV(addr, net::make_sockaddr<traits_type>(ip, port));
      TRV(sock, socket(addr));
      ENSURE(sock.reuse_addr());
      ENSURE(sock.bind(addr));
      return std::move(sock);
    }
    template <class... Flags>  /// TODO: add attr opt for socket
    Expected<SocketCompose<Traits, Flags...>> c2(const char* ip, inet::port_t port) noexcept {
      using traits_type = SocketTraits<Traits, Flags...>;
      TRV(addr, net::make_sockaddr<traits_type>(ip, port));
      TRV(sock, socket(addr));
      ENSURE(sock.connect(addr));
      return std::move(sock);
    }
  };
}  // namespace _detail

template <class Traits>
struct SocketCreator {
  using io_dev_type = net::Socket<Traits>;
  template <class... Flags>  /// TODO: add attr opt for socket
  Expected<net::SocketCompose<Traits, Flags...>> c_b(const char* ip, inet::port_t port) noexcept;
};

template <>
struct SocketCreator<net::TcpIpv4SocketTraits>
    : _detail::TcpIpSocketCreator<net::TcpIpv4SocketTraits> {};

template <>
struct SocketCreator<net::TcpIpSocketTraits> : _detail::TcpIpSocketCreator<net::TcpIpSocketTraits> {
};

template <>
struct SocketCreator<net::TcpIpv6SocketTraits>
    : _detail::TcpIpSocketCreator<net::TcpIpv6SocketTraits> {};

template <>
struct SocketCreator<net::UdpIpv4SocketTraits>
    : _detail::UdpIpSocketCreator<net::UdpIpv4SocketTraits> {};

template <>
struct SocketCreator<net::UdpIpSocketTraits> : _detail::UdpIpSocketCreator<net::UdpIpSocketTraits> {
};

template <class... Flags>
consteval auto make_socket_utils() {
  return SocketCreator<net::SocketTraits<Flags...>>();
}
XSL_SYS_NE
#endif
