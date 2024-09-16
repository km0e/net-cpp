/**
 * @file conn.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Connection based functions
 * @version 0.2
 * @date 2024-08-19
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_CONN
#  define XSL_SYS_NET_CONN
#  include "xsl/coro.h"
#  include "xsl/sys/def.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/sockaddr.h"
#  include "xsl/sys/raw.h"
#  include "xsl/sys/sync.h"

#  include <expected>
XSL_SYS_NET_NB

namespace {
  template <class Traits>
  inline Task<std::expected<net::AsyncReadWriteSocket<Traits>, errc>> raw_connect(
      net::ReadWriteSocket<Traits> &&skt, const sockaddr &sa, socklen_t len, Poller &poller) {
    auto ec = errc{};
    if (filter_interrupt(::connect, skt.raw(), &sa, len) != 0) {
      if (errno != EINPROGRESS) [[unlikely]] {
        ec = errc{errno};
      } else {
        auto async_skt = std::move(skt).async(poller);
        if (!co_await async_skt.write_signal()) {
          skt = std::move(async_skt).sync(poller);
          co_return std::unexpected{errc::not_connected};
        }
        auto check = [](int fd) {
          int opt;
          socklen_t len = sizeof(opt);
          if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &opt, &len) == -1) [[unlikely]] {
            LOG2("Failed to getsockopt: {}", strerror(errno));
            return errno;
          }
          if (opt != 0) [[unlikely]] {
            LOG2("Failed to connect: {}", strerror(opt));
            return opt;
          }
          return 0;
        };
        int res = check(async_skt.raw());
        if (res != 0) [[unlikely]] {
          skt = std::move(async_skt).sync(poller);
          co_return std::unexpected{errc{res}};
        }
        LOG5("Connected to fd: {}", async_skt.raw());
        co_return std::move(async_skt);
      }
    }
    co_return std::unexpected{ec};
  }
}  // namespace

template <class Traits>
struct ConnectionUtils {
  /// @brief Connect to a address
  errc connect(this ReadWriteSocket<Traits> &self, const SockAddr<Traits> &sa) {
    auto [addr, addrlen] = sa.raw();
    return check_ec(filter_interrupt(::connect, self.raw(), &addr, addrlen));
  }
  /// @brief Bind to a address
  constexpr errc bind(this ReadWriteSocket<Traits> &self, const SockAddr<Traits> &sa) {
    auto [addr, addrlen] = sa.raw();
    return check_ec(::bind(self.raw(), &addr, addrlen));
  }
};

template <ConnectionBasedSocketTraits Traits>
struct ConnectionUtils<Traits> {
  /// @brief Async connect to a address
  Task<std::expected<net::AsyncReadWriteSocket<Traits>, errc>> async_connect(
      this ReadWriteSocket<Traits> &&self, const SockAddr<Traits> &sa, Poller &poller) {
    auto [addr, addrlen] = sa.raw();
    co_return co_await raw_connect(std::move(self), addr, addrlen, poller);
  }
  /// @brief Bind to a address
  constexpr errc bind(this AsyncReadWriteSocket<Traits> &self, const SockAddr<Traits> &sa) {
    auto [addr, addrlen] = sa.raw();
    return check_ec(::bind(self.raw(), &addr, addrlen));
  }
  /// @brief Bind to a address
  constexpr errc bind(this ReadWriteSocket<Traits> &self, const SockAddr<Traits> &sa) {
    auto [addr, addrlen] = sa.raw();
    return check_ec(::bind(self.raw(), &addr, addrlen));
  }
  /// @brief Accept a connection
  constexpr std::expected<ReadWriteSocket<Traits>, errc> accept(this ReadWriteSocket<Traits> &self,
                                                                SockAddr<Traits> *addr = nullptr) {
    return ConnectionUtils::accept(self.raw(), addr);
  }
  /// @brief Accept a connection
  Task<std::expected<ReadWriteSocket<Traits>, errc>> accept(this AsyncReadWriteSocket<Traits> &self,
                                                            SockAddr<Traits> *addr = nullptr) {
    while (true) {
      auto res = ConnectionUtils::accept(self.raw(), addr);
      if (res) {
        co_return std::move(*res);
      } else if (res.error() == errc::resource_unavailable_try_again
                 || res.error() == errc::operation_would_block) {
        if (!co_await self.read_signal()) {
          co_return std::unexpected{errc::not_connected};
        }
      } else {
        co_return std::unexpected{res.error()};
      }
    }
  }
  /// @brief Start listening
  constexpr errc listen(this auto &&self, int max_connections = 128) {
    return check_ec(::listen(self.raw(), max_connections));
  }

private:
  static constexpr std::expected<ReadWriteSocket<Traits>, errc> accept(RawHandle _raw,
                                                                       SockAddr<Traits> *addr) {
    auto tmp_fd = [_raw, addr] {
      if (addr == nullptr) {
        return ::accept4(_raw, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
      } else {
        auto [sockaddr, addrlen] = addr->raw();
        return ::accept4(_raw, &sockaddr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
      }
    }();
    if (tmp_fd < 0) {
      INFO("Failed to accept: {}", strerror(errno));
      return std::unexpected{errc(errno)};
    }
    LOG5("accept socket {}", tmp_fd);
    // char ip[NI_MAXHOST], port[NI_MAXSERV];
    // if (getnameinfo(&addr, addrlen, ip, NI_MAXHOST, port, NI_MAXSERV, NI_NUMERICHOST |
    // NI_NUMERICSERV)
    //     != 0) {
    //   return std::unexpected{errc(errno)};
    // }
    return ReadWriteSocket<Traits>(tmp_fd);
  }
};

XSL_SYS_NET_NE
#endif
