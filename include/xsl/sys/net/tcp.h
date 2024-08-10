#pragma once
#ifndef XSL_SYS_NET_TCP
#  define XSL_SYS_NET_TCP
#  include "xsl/coro/task.h"
#  include "xsl/sync/poller.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/endpoint.h"
#  include "xsl/sys/net/socket.h"
#  include "xsl/sys/raw.h"

#  include <expected>
#  include <memory>
XSL_SYS_NET_NB

namespace impl_connect {
  template <class Traits, class Executor = coro::ExecutorBase>
  coro::Task<std::expected<AsyncSocket<Traits>, std::errc>, Executor> connect(addrinfo *ai,
                                                                              Poller &poller) {
    int tmp_fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (tmp_fd == -1) [[unlikely]] {
      co_return std::unexpected{std::errc{errno}};
    }
    LOG5("Created fd: {}", tmp_fd);
    if (auto snb_res = set_blocking<false>(tmp_fd); !snb_res) [[unlikely]] {
      close(tmp_fd);
      co_return std::unexpected{snb_res.error()};
    }
    LOG5("Set non-blocking to fd: {}", tmp_fd);
    int ec = ::connect(tmp_fd, ai->ai_addr, ai->ai_addrlen);
    auto write_sem = std::make_shared<coro::CountingSemaphore<1>>();
    if (ec != 0) {
      LOG3("Failed to connect to fd: {}", tmp_fd);
      poller.add(tmp_fd, sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
                 sync::PollCallback<sync::PollTraits, sync::IOM_EVENTS::OUT>{write_sem});
      co_await *write_sem;
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
      int res = check(tmp_fd);
      if (res != 0) [[unlikely]] {
        close(tmp_fd);
        poller.remove(tmp_fd);
        co_return std::unexpected{std::errc{res}};
      }
    }
    LOG5("Connected to fd: {}", tmp_fd);
    auto read_sem = std::make_shared<coro::CountingSemaphore<1>>();
    poller.modify(tmp_fd, sync::IOM_EVENTS::IN | sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
                  sync::PollCallback<sync::PollTraits, sync::IOM_EVENTS::IN, sync::IOM_EVENTS::OUT>{
                      read_sem, write_sem});
    co_return AsyncSocket<Traits>{read_sem, write_sem, tmp_fd};
  }
}  // namespace impl_connect

template <class Traits>
using ConnectResult = std::expected<AsyncSocket<Traits>, std::errc>;

template <class Executor = coro::ExecutorBase, class Traits>
inline decltype(auto) connect(const Endpoint<Traits> &ep, Poller &poller) {
  return impl_connect::connect<Traits, Executor>(ep.raw(), poller);
}

template <class Executor = coro::ExecutorBase, class Traits>
coro::Task<ConnectResult<Traits>, Executor> connect(const EndpointSet<Traits> &eps,
                                                    Poller &poller) {
  for (auto &ep : eps) {
    auto res = co_await impl_connect::connect<Traits, Executor>(ep.raw(), poller);
    if (res) {
      co_return std::move(*res);
    }
  }
  co_return std::unexpected{std::errc{errno}};
}

template <class Traits>
using BindResult = std::expected<Socket<Traits>, std::errc>;

namespace impl_bind {
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
    if (bind(tmp_fd, ai->ai_addr, ai->ai_addrlen) == -1) [[unlikely]] {
      close(tmp_fd);
      return std::unexpected{std::errc{errno}};
    }
    return tmp_fd;
  }
}  // namespace impl_bind

template <class Traits>
BindResult<Traits> bind(const Endpoint<Traits> &ep) {
  return impl_bind::bind(ep.raw()).transform([](int fd) { return Socket<Traits>(fd); });
}
template <class Traits>
BindResult<Traits> bind(const EndpointSet<Traits> &eps) {
  for (auto &ep : eps) {
    auto bind_res = impl_bind::bind(ep.raw());
    if (bind_res) {
      return Socket<Traits>{*bind_res};
    }
  }
  return std::unexpected{std::errc{errno}};
}

template <SocketLike S>
std::expected<void, std::errc> listen(S &skt, int max_connections = 10) {
  if (::listen(skt.raw(), max_connections) == -1) {
    return std::unexpected{std::errc{errno}};
  }
  return {};
}

XSL_SYS_NET_NE
#endif
