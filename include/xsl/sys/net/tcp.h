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
SYS_NET_NB

namespace impl_connect {
  template <class Executor = coro::ExecutorBase>
  coro::Task<std::expected<int, std::errc>, Executor> connect(addrinfo *ai, Poller &poller) {
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
    if (ec != 0) {
      LOG3("Failed to connect to fd: {}", tmp_fd);
      auto sem = std::make_shared<coro::CountingSemaphore<1>>();
      poller.add(tmp_fd, sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
                 sync::PollCallback<sync::IOM_EVENTS::OUT>{sem});
      co_await *sem;
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
        co_return std::unexpected{std::errc{res}};
      }
    }
    LOG5("Connected to fd: {}", tmp_fd);
    co_return tmp_fd;
  }
}  // namespace impl_connect

using ConnectResult = std::expected<Socket, std::errc>;

template <class Executor = coro::ExecutorBase>
inline decltype(auto) connect(const Endpoint &ep, Poller &poller) {
  return impl_connect::connect<Executor>(ep.raw(), poller).transform([](auto &&exp) {
    exp.transform([](auto fd) { return Socket(fd); });
  });
}

template <class Executor = coro::ExecutorBase>
coro::Task<ConnectResult, Executor> connect(const EndpointSet &eps, Poller &poller) {
  for (auto &ep : eps) {
    auto res = co_await impl_connect::connect<Executor>(ep.raw(), poller);
    if (res) {
      co_return Socket(*res);
    }
  }
  co_return std::unexpected{std::errc{errno}};
}

using BindResult = std::expected<Socket, std::errc>;

BindResult bind(const Endpoint &ep);
BindResult bind(const EndpointSet &eps);

std::expected<void, std::errc> listen(Socket &skt, int max_connections = 10);

SYS_NET_NE
#endif
