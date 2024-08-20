/**
 * @file connect.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_CONNECT
#  define XSL_SYS_NET_CONNECT
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/endpoint.h"
#  include "xsl/sys/net/socket.h"

#  include <netdb.h>
#  include <sys/socket.h>
#  include <unistd.h>

#  include <expected>
#  include <memory>
#  include <tuple>
XSL_SYS_NET_NB

namespace impl_connect {

  template <class PollTraits, class Executor = coro::ExecutorBase>
  Task<std::expected<
           std::tuple<std::shared_ptr<CountingSemaphore<1>>, std::shared_ptr<CountingSemaphore<1>>>,
           std::errc>,
       Executor>
  blocking_connect(int fd, Poller &poller) {
    auto write_sem = std::make_shared<CountingSemaphore<1>>();
    LOG3("Failed to connect to fd: {}", fd);
    poller.add(fd, PollForCoro<PollTraits, IOM_EVENTS::OUT>{write_sem});
    if (!co_await *write_sem) {
      co_return std::unexpected{std::errc{errno}};
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
    int res = check(fd);
    if (res != 0) [[unlikely]] {
      close(fd);
      poller.remove(fd);
      co_return std::unexpected{std::errc{res}};
    }
    LOG5("Connected to fd: {}", fd);
    auto read_sem = std::make_shared<CountingSemaphore<1>>();
    poller.modify(fd,
                  PollForCoro<PollTraits, IOM_EVENTS::IN, IOM_EVENTS::OUT>{read_sem, write_sem});
    co_return std::make_tuple(read_sem, write_sem);
  }

  template <class Traits, class Executor = coro::ExecutorBase>
  Task<std::expected<AsyncSocket<Traits>, std::errc>, Executor> raw_connect(Socket<Traits> &&skt,
                                                                            sockaddr *sa,
                                                                            socklen_t len,
                                                                            Poller &poller) {
    int ec = filter_interrupt(::connect, skt.raw(), sa, len);
    if (ec != 0) {
      if (errno != EINPROGRESS) [[unlikely]] {
        co_return std::unexpected{std::errc{errno}};
      }
      auto res = co_await blocking_connect<typename Traits::poll_traits_type, Executor>(skt.raw(),
                                                                                        poller);
      if (!res) {
        co_return std::unexpected{res.error()};
      }
      auto [read_sem, write_sem] = std::move(*res);
      co_return AsyncSocket<Traits>{std::move(read_sem), std::move(write_sem), skt.raw()};
    } else {
      co_return std::move(skt).async(poller);
    }
  }

  template <class Traits, class Executor = coro::ExecutorBase>
  Task<std::expected<AsyncSocket<Traits>, std::errc>, Executor> connect(addrinfo *ai,
                                                                        Poller &poller) {
    auto skt = make_socket<Socket<Traits>>();
    if (!skt) {
      co_return std::unexpected{skt.error()};
    }
    LOG5("Created fd: {}", skt->raw());
    if (auto snb_res = set_blocking<false>(skt->raw()); !snb_res) [[unlikely]] {
      close(skt->raw());
      co_return std::unexpected{snb_res.error()};
    }
    co_return co_await raw_connect<Traits, Executor>(std::move(*skt), ai->ai_addr, ai->ai_addrlen,
                                                     poller);
  }
  template <class Traits>
    requires(!CSocketTraits<Traits>)
  std::expected<Socket<Traits>, std::errc> connect(addrinfo *ai) {
    auto skt = make_socket<Socket<Traits>>();
    if (!skt) {
      return std::unexpected{skt.error()};
    }
    LOG5("Created fd: {}", skt->raw());
    int ec = filter_interrupt(::connect, skt->raw(), ai->ai_addr, ai->ai_addrlen);
    if (ec != 0) {
      return std::unexpected{std::errc{errno}};
    }
    if (auto snb_res = set_blocking<false>(skt->raw()); !snb_res) [[unlikely]] {
      close(skt->raw());
      return std::unexpected{snb_res.error()};
    }
    return std::move(*skt);
  }
  template <class Traits, class Executor = coro::ExecutorBase>
  Task<std::expected<AsyncSocket<Traits>, std::errc>, Executor> connect(const SockAddr<Traits> sa,
                                                                        Poller &poller) {
    auto skt = make_socket<Socket<Traits>>();
    if (!skt) {
      co_return std::unexpected{skt.error()};
    }
    LOG5("Created fd: {}", skt->raw());
    if (auto snb_res = set_blocking<false>(skt->raw()); !snb_res) [[unlikely]] {
      close(skt->raw());
      co_return std::unexpected{snb_res.error()};
    }
    LOG5("Set non-blocking to fd: {}", skt->raw());
    auto [addr, addrlen] = sa.raw();
    co_return co_await raw_connect<Traits, Executor>(std::move(*skt), addr, addrlen, poller);
  }
}  // namespace impl_connect

template <class Traits>
using ConnectResult = std::expected<AsyncSocket<Traits>, std::errc>;

template <class Executor = coro::ExecutorBase, class Traits>
inline decltype(auto) connect(const SockAddr<Traits> &sa, Poller &poller) {
  return impl_connect::connect<Traits, Executor>(sa, poller);
}

template <class Executor = coro::ExecutorBase, class Traits>
inline decltype(auto) connect(const Endpoint<Traits> &ep, Poller &poller) {
  return impl_connect::connect<Traits, Executor>(ep.raw(), poller);
}

template <class Executor = coro::ExecutorBase, class Traits>
  requires(!CSocketTraits<Traits>)
inline decltype(auto) connect(const Endpoint<Traits> &ep) {
  return impl_connect::connect<Traits>(ep.raw());
}

template <class Executor = coro::ExecutorBase, class Traits>
  requires(!CSocketTraits<Traits>)
inline decltype(auto) connect(const EndpointSet<Traits> &eps) {
  std::expected<Socket<Traits>, std::errc> res{std::unexpect, std::errc{0}};
  for (auto &ep : eps) {
    res = impl_connect::connect<Traits>(ep.raw());
    if (res) {
      return res;
    }
  }
  return res;
}

template <class Executor = coro::ExecutorBase, class Traits>
Task<ConnectResult<Traits>, Executor> connect(const EndpointSet<Traits> &eps, Poller &poller) {
  for (auto &ep : eps) {
    auto res = co_await impl_connect::connect<Traits, Executor>(ep.raw(), poller);
    if (res) {
      co_return std::move(*res);
    }
  }
  co_return std::unexpected{std::errc{errno}};
}
XSL_SYS_NET_NE
#endif
