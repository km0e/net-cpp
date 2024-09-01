/**
 * @file utils.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
 * @date 2024-08-19
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_UTILS_H
#  define XSL_SYS_NET_UTILS_H
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/endpoint.h"
#  include "xsl/sys/net/socket.h"
#  include "xsl/sys/raw.h"

#  include <netdb.h>
#  include <sys/socket.h>
#  include <unistd.h>

#  include <expected>
#  include <system_error>
XSL_SYS_NET_NB

template <class Traits>
using BindResult = std::expected<Socket<Traits>, std::errc>;

namespace {
  static inline std::expected<int, std::errc> bind(addrinfo *ai) {
    int tmp_fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (tmp_fd == -1) [[unlikely]] {
      return std::unexpected{std::errc{errno}};
    }
    LOG5("Created fd: {}", tmp_fd);
    if (auto snb_res = set_blocking<false>(tmp_fd); !snb_res) [[unlikely]] {
      close(tmp_fd);
      return std::unexpected{snb_res.error()};
    }
    LOG5("Set non-blocking to fd: {}", tmp_fd);
    int opt = 1;
    if (setsockopt(tmp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) [[unlikely]] {
      close(tmp_fd);
      return std::unexpected{std::errc{errno}};
    }
    LOG5("Set reuse addr to fd: {}", tmp_fd);
    if (::bind(tmp_fd, ai->ai_addr, ai->ai_addrlen) == -1) [[unlikely]] {
      close(tmp_fd);
      return std::unexpected{std::errc{errno}};
    }
    return tmp_fd;
  }
}  // namespace
/// @brief Bind to an endpoint
template <class Traits>
BindResult<Traits> bind(const Endpoint<Traits> &ep) {
  return bind(ep.raw()).transform([](int fd) { return Socket<Traits>(fd); });
}
/// @brief Bind to an endpoint set
template <class Traits>
BindResult<Traits> bind(const EndpointSet<Traits> &eps) {
  for (auto &ep : eps) {
    auto bind_res = bind(ep.raw());
    if (bind_res) {
      return Socket<Traits>(*bind_res);
    }
  }
  return std::unexpected{std::errc{errno}};
}

XSL_SYS_NET_NE
#endif
