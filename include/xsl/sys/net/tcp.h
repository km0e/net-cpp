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

using AcceptResult = std::expected<std::tuple<Socket, SockAddr>, std::errc>;

AcceptResult accept(int fd);

using ConnectResult = std::expected<Socket, std::errc>;

template <class Executor = coro::ExecutorBase>
coro::Task<std::expected<int, std::errc>, Executor> connect(addrinfo *ai,
                                                            std::shared_ptr<Poller> &poller) {
  int tmpfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  if (tmpfd == -1) [[unlikely]] {
    co_return std::unexpected{std::errc{errno}};
  }
  LOG5("Created fd: {}", tmpfd);
  if (auto snb_res = set_blocking<false>(tmpfd); !snb_res) [[unlikely]] {
    close(tmpfd);
    co_return std::unexpected{snb_res.error()};
  }
  LOG5("Set non-blocking to fd: {}", tmpfd);
  int ec = ::connect(tmpfd, ai->ai_addr, ai->ai_addrlen);
  if (ec != 0) {
    LOG3("Failed to connect to fd: {}", tmpfd);
    auto sem = std::make_shared<coro::CountingSemaphore<1>>();
    poller->add(tmpfd, sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
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
    int res = check(tmpfd);
    if (res != 0) [[unlikely]] {
      close(tmpfd);
      co_return std::unexpected{std::errc{res}};
    }
  }
  LOG5("Connected to fd: {}", tmpfd);
  co_return tmpfd;
}
template <class Executor = coro::ExecutorBase>
inline decltype(auto) connect(const Endpoint &ep, std::shared_ptr<Poller> poller) {
  return connect<Executor>(ep.raw(), poller).transform([](auto &&exp) {
    exp.transform([](int fd) { return Socket(fd); });
  });
}
template <class Executor = coro::ExecutorBase>
coro::Task<ConnectResult, Executor> connect(const EndpointSet &eps,
                                            std::shared_ptr<Poller> poller) {
  for (auto &ep : eps) {
    auto res = co_await connect<Executor>(ep.raw(), poller);
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
