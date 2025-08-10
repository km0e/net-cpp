/**
 * @file gai.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Get address info
 * @version 0.2.0
 * @date 2024-09-10
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_GAI
#  define XSL_ASIO_GAI
#  include <netdb.h>
#  include <xsl/asio/def.h>
#  include <xsl/asio/socket.h>
#  include <xsl/io.h>
#  include <xsl/sys.h>

#  include <utility>
XSL_ASIO_NB

namespace {
  template <class Traits>
  inline Task<std::expected<AsyncSocket<Traits>, errc>> raw_connect(sys::net::Socket<Traits> &&skt,
                                                                    const sockaddr &sa,
                                                                    socklen_t len, Context &ctx) {
    auto ec = errc{};
    if (sys::filter_interrupt(::connect, skt.raw(), &sa, len) != 0) {
      if (errno != EINPROGRESS) [[unlikely]] {
        ec = errc{errno};
      } else {
        auto async_skt = AsyncSocket(ctx, std::move(skt));
        if (!co_await async_skt.write_signal()) {
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
        int res = check(async_skt.raw());
        if (res != 0) [[unlikely]] {
          // skt = std::move(async_skt).sync(poller);
          co_return std::unexpected{errc{res}};
        }
        log_debug("Connected to fd: {}", async_skt.raw());
        co_return std::move(async_skt);
      }
    }
    co_return std::unexpected{ec};
  }
}  // namespace

/**
 * @brief Connect to a remote address
 *
 * @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp, TcpIpv4 ...
 * @tparam Traits
 * @tparam Args
 * @param poller The poller
 * @param args The arguments for getaddrinfo
 * @return Task<std::expected<AsyncSocket<Traits>, std::error_condition>>
 */
template <class... Flags, typename Traits = sys::net::SocketTraits<Flags...>, typename... Args>
Task<Expected<AsyncSocket<Traits>>> gai_async_connect(Context &ctx, Args &&...args) {
  CO_TRV(res_resolved, sys::net::getaddrinfo<Traits>(std::forward<Args>(args)...));
  using Skt = sys::net::Socket<Traits>;
  errc ec;
  for (auto &ai : res_resolved) {
    Skt skt(ai.ai_family, ai.ai_socktype, ai.ai_protocol);
    if (!skt.is_valid()) {
      continue;
    }
    auto res_skt = co_await raw_connect(std::move(skt), *ai.ai_addr, ai.ai_addrlen, ctx);
    if (res_skt) {
      co_return std::move(*res_skt);
    }
  }
  co_return std::unexpected{errc{ec}};
}

XSL_ASIO_NE
#endif
