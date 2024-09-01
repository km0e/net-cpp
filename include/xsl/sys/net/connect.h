/**
 * @file connect.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Connect to a remote endpoint
 * @version 0.11
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_CONNECT
#  define XSL_SYS_NET_CONNECT
#  include "xsl/coro.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/endpoint.h"
#  include "xsl/sys/net/socket.h"
#  include "xsl/sys/raw.h"
#  include "xsl/sys/sync.h"

#  include <netdb.h>
#  include <sys/socket.h>
#  include <unistd.h>

#  include <expected>
#  include <tuple>
XSL_SYS_NET_NB

namespace {
  template <class PollTraits>
  Task<std::expected<std::tuple<SignalReceiver<>, SignalReceiver<>>, std::errc>> blocking_connect(
      int fd, Poller &poller) {
    auto [write_signal] = poll_by_signal<PollTraits>(poller, fd, IOM_EVENTS::OUT);
    if (!co_await write_signal) {
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
    PubSub<IOM_EVENTS, 1> pubsub{};
    auto read_signal = *pubsub.subscribe(IOM_EVENTS::IN);
    write_signal = *pubsub.subscribe(IOM_EVENTS::OUT);
    poller.modify(fd, IOM_EVENTS::IN | IOM_EVENTS::OUT | IOM_EVENTS::ET,
                  PollForCoro<PollTraits>{std::move(pubsub)});
    co_return std::make_tuple(std::move(read_signal), std::move(write_signal));
  }

  template <class Traits>
  Task<std::expected<AsyncSocket<Traits>, std::errc>> raw_connect(Socket<Traits> &&skt,
                                                                  sockaddr *sa, socklen_t len,
                                                                  Poller &poller) {
    int ec = filter_interrupt(::connect, skt.raw(), sa, len);
    if (ec != 0) {
      if (errno != EINPROGRESS) [[unlikely]] {
        co_return std::unexpected{std::errc{errno}};
      }
      auto res = co_await blocking_connect<typename Traits::poll_traits_type>(skt.raw(), poller);
      if (!res) {
        co_return std::unexpected{res.error()};
      }
      auto [read_sig, write_sig] = std::move(*res);
      co_return AsyncSocket<Traits>{skt.raw(), std::move(read_sig), std::move(write_sig)};
    } else {
      co_return std::move(skt).async(poller);
    }
  }

  template <class Traits>
  Task<std::expected<AsyncSocket<Traits>, std::errc>> connect(addrinfo *ai, Poller &poller) {
    auto skt = make_socket<Socket<Traits>>();
    if (!skt) {
      co_return std::unexpected{skt.error()};
    }
    LOG5("Created fd: {}", skt->raw());
    if (auto snb_res = set_blocking<false>(skt->raw()); !snb_res) [[unlikely]] {
      close(skt->raw());
      co_return std::unexpected{snb_res.error()};
    }
    co_return co_await raw_connect<Traits>(std::move(*skt), ai->ai_addr, ai->ai_addrlen, poller);
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
  template <class Traits>
  Task<std::expected<AsyncSocket<Traits>, std::errc>> connect(const SockAddr<Traits> sa,
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
    co_return co_await raw_connect<Traits>(std::move(*skt), addr, addrlen, poller);
  }
}  // namespace

template <class Traits>
using ConnectResult = std::expected<AsyncSocket<Traits>, std::errc>;

/**
 * @brief connect to a remote endpoint
 *
 * @tparam Traits the socket traits
 * @param sa the socket address
 * @param poller the poller
 * @return decltype(auto) the result
 */
template <class Traits>
inline decltype(auto) connect(const SockAddr<Traits> &sa, Poller &poller) {
  return connect<Traits>(sa, poller);
}
/**
 * @brief connect to a remote endpoint
 *
 * @tparam Traits
 * @param ep
 * @param poller
 * @return decltype(auto)
 */
template <class Traits>
inline decltype(auto) connect(const Endpoint<Traits> &ep, Poller &poller) {
  return connect<Traits>(ep.raw(), poller);
}
///TODO: for dns resolve
template <class Traits>
  requires(!CSocketTraits<Traits>)
inline decltype(auto) connect(const Endpoint<Traits> &ep) {
  return connect<Traits>(ep.raw());
}

template <class Traits>
  requires(!CSocketTraits<Traits>)
inline decltype(auto) connect(const EndpointSet<Traits> &eps) {
  std::expected<Socket<Traits>, std::errc> res{std::unexpect, std::errc{0}};
  for (auto &ep : eps) {
    res = connect<Traits>(ep.raw());
    if (res) {
      return res;
    }
  }
  return res;
}

template <class Traits>
Task<ConnectResult<Traits>> connect(const EndpointSet<Traits> &eps, Poller &poller) {
  for (auto &ep : eps) {
    auto res = co_await connect<Traits>(ep.raw(), poller);
    if (res) {
      co_return std::move(*res);
    }
  }
  co_return std::unexpected{std::errc{errno}};
}
XSL_SYS_NET_NE
#endif
