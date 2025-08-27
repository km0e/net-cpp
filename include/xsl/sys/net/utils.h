/**
 * @file utils.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief socket utilities
 * @version 0.2.0
 * @date 2025-06-07
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_SYS_NET_UTILS
#  define XSL_SYS_NET_UTILS
#  include <netdb.h>
#  include <xsl/error.h>
#  include <xsl/feature.h>
#  include <xsl/sys/net/def.h>
#  include <xsl/sys/net/gai.h>
#  include <xsl/sys/net/socket.h>
#  include <xsl/sys/raw.h>

#  include <cerrno>
#  include <utility>
XSL_SYS_NET_NB

/**
 * @brief Connect to a remote address
 *
 * @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp, TcpIpv4 ...
 * @tparam Traits
 * @tparam Args
 * @param args The arguments for getaddrinfo
 * @return Expected<Socket<Traits>>
 */

template <class... Flags, ConnectionLessSocketTraits Traits = SocketTraits<Flags...>, class... Args>
constexpr Expected<Socket<Traits>> gai_connect(Args &&...args) {
  TRV(res_resolved, getaddrinfo<Flags...>(std::forward<Args>(args)...));
  auto skt = Socket<Traits>();
  ENSURE(skt.is_valid(), errno);
  int ec = 0;
  for (auto &ai : res_resolved) {
    ec = filter_interrupt(::connect, skt.raw(), ai.ai_addr, ai.ai_addrlen);
    if (ec == 0) {
      return std::move(skt);
    }
  }
  RETURN(ec);
}

/**
 * @brief Bind to a local address
 *
 * @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp, TcpIpv4 ...
 * @tparam Traits
 * @tparam Args
 * @param args The arguments for getaddrinfo, such as ("0.0.0.0", "8080"), ("::", "8080"),
 * ("localhost", "8080"), etc.
 * @return std::expected<Socket<Traits>, std::error_condition>
 */
template <class... Flags, class Traits = SocketTraits<Flags...>, class... Args>
Expected<Socket<Traits>> gai_bind(Args &&...args) {
  TRV(res_resolved, getaddrinfo<Flags...>(std::forward<Args>(args)...));
  using Skt = Socket<Traits>;
  int ec{};
  for (auto &ai : res_resolved) {
    Skt skt(ai.ai_family, ai.ai_socktype, ai.ai_protocol);
    if (!skt.is_valid()) {
      log_warning("Failed to create socket, err: {}", std::strerror(errno));
      continue;
    }
    log_debug("Set non-blocking to fd: {}", skt.raw());
    int opt = 1;
    if (setsockopt(skt.raw(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
      continue;
    }
    log_debug("Set reuse addr to fd: {}", skt.raw());
    if (::bind(skt.raw(), ai.ai_addr, ai.ai_addrlen) == 0) {
      return std::move(skt);
    }
    ec = errno;
  }
  RETURN(ec);
}

template <class Traits>
struct TcpIpSocketUtils : Traits {
  using io_dev_type = Socket<Traits>;

  template <class... Flags>  /// TODO: add attr opt for socket
  Expected<SocketCompose<Traits, Flags...>> c(const char *ip, inet::port_t port) noexcept {
    using traits_type = SocketTraits<Traits, Flags...>;
    TRV(addr, make_sockaddr<traits_type>(ip, port));
    TRV(sock, socket(addr));
    ENSURE(sock.reuse_addr());
    ENSURE(sock.bind(addr));
    return std::move(sock);
  }
};

template <class Traits>
struct UdpIpSocketUtils : Traits {
  using io_dev_type = Socket<Traits>;

  template <class... Flags>  /// TODO: add attr opt for socket
  Expected<SocketCompose<Traits, Flags...>> c(const char *ip, inet::port_t port) noexcept {
    using traits_type = SocketTraits<Traits, Flags...>;
    TRV(addr, make_sockaddr<traits_type>(ip, port));
    TRV(sock, socket(addr));
    ENSURE(sock.reuse_addr());
    ENSURE(sock.bind(addr));
    return std::move(sock);
  }
  template <class... Flags>  /// TODO: add attr opt for socket
  Expected<SocketCompose<Traits, Flags...>> c2(const char *ip, inet::port_t port) noexcept {
    using traits_type = SocketTraits<Traits, Flags...>;
    TRV(addr, make_sockaddr<traits_type>(ip, port));
    TRV(sock, socket(addr));
    ENSURE(sock.connect(addr));
    return std::move(sock);
  }
};

template <class Traits>
struct SocketUtils {
  using io_dev_type = Socket<Traits>;
  template <class... Flags>  /// TODO: add attr opt for socket
  Expected<SocketCompose<Traits, Flags...>> c(const char *ip, inet::port_t port) noexcept;
};

template <>
struct SocketUtils<TcpIpv4SocketTraits> : TcpIpSocketUtils<TcpIpv4SocketTraits> {};

template <>
struct SocketUtils<TcpIpSocketTraits> : TcpIpSocketUtils<TcpIpSocketTraits> {};

template <>
struct SocketUtils<TcpIpv6SocketTraits> : TcpIpSocketUtils<TcpIpv6SocketTraits> {};

template <>
struct SocketUtils<UdpIpv4SocketTraits> : UdpIpSocketUtils<UdpIpv4SocketTraits> {};

template <>
struct SocketUtils<UdpIpSocketTraits> : UdpIpSocketUtils<UdpIpSocketTraits> {};

template <class... Flags>
consteval auto make_socket_utils() {
  return SocketUtils<net::SocketTraits<Flags...>>();
}

XSL_SYS_NET_NE
#endif
