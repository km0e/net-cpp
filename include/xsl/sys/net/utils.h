/**
 * @file utils.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief socket utilities
 * @version 0.1.1
 * @date 2025-06-07
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_SYS_NET_UTILS
#  define XSL_SYS_NET_UTILS
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/socket.h"
#  include "xsl/sys/raw.h"

#  include <netdb.h>

#  include <cerrno>
#  include <system_error>
#  include <utility>
XSL_SYS_NET_NB

/**
 * @brief Connect to a remote address
 *
 * @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp, TcpIpv4 ...
 * @tparam Traits
 * @tparam Args
 * @param args The arguments for getaddrinfo
 * @return std::expected<Socket<Traits>, std::error_condition>
 */

template <class... Flags, ConnectionLessSocketTraits Traits = SocketTraits<Flags...>, class... Args>
constexpr std::expected<Socket<Traits>, std::error_condition> gai_connect(Args &&...args) {
  auto res_resolved = getaddrinfo<Flags...>(std::forward<Args>(args)...);
  if (!res_resolved) {
    return std::unexpected{res_resolved.error()};
  }
  auto skt = Socket<Traits>();
  if (!skt.is_valid()) {
    return std::unexpected{current_ec()};
  }
  int ec = 0;
  for (auto &ai : *res_resolved) {
    ec = filter_interrupt(::connect, skt.raw(), ai.ai_addr, ai.ai_addrlen);
    if (ec == 0) {
      return std::move(skt);
    }
  }
  return std::unexpected{errc{errno}};
}

/**
 * @brief Bind to a local address
 *
 * @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp, TcpIpv4 ...
 * @tparam Traits
 * @tparam Args
 * @param args The arguments for getaddrinfo
 * @return std::expected<Socket<Traits>, std::error_condition>
 */
template <class... Flags, class Traits = SocketTraits<Flags...>, class... Args>
std::expected<Socket<Traits>, std::error_condition> gai_bind(Args &&...args) {
  auto res_resolved = getaddrinfo<Flags...>(std::forward<Args>(args)...);
  if (!res_resolved) {
    return std::unexpected{res_resolved.error()};
  }
  using Skt = Socket<Traits>;
  errc ec{};
  for (auto &ai : *res_resolved) {
    Skt skt(ai.ai_family, ai.ai_socktype, ai.ai_protocol);
    if (!skt.is_valid()) {
      log_warning("Failed to create socket, err: {}", current_ec().message());
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
    ec = errc{errno};
  }
  return std::unexpected{std::make_error_condition(ec)};
}

XSL_SYS_NET_NE
#endif
