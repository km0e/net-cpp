#include "xsl/coro/semaphore.h"
#include "xsl/coro/task.h"
#include "xsl/logctl.h"
#include "xsl/sync.h"
#include "xsl/sys/net/def.h"
#include "xsl/sys/net/endpoint.h"
#include "xsl/sys/net/socket.h"
#include "xsl/sys/net/tcp.h"
#include "xsl/sys/raw.h"

#include <netdb.h>
#include <sys/io.h>
#include <sys/raw.h>
#include <sys/socket.h>

#include <expected>
#include <system_error>
#include <tuple>
SYS_NET_NB

AcceptResult accept(int fd) {
  SockAddr addr{};
  auto [sockaddr, addrlen] = addr.raw();
  int tmpfd = ::accept4(fd, sockaddr, addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (tmpfd < 0) {
    return std::unexpected{std::errc(errno)};
  }
  LOG5("accept socket {}", tmpfd);
  // char ip[NI_MAXHOST], port[NI_MAXSERV];
  // if (getnameinfo(&addr, addrlen, ip, NI_MAXHOST, port, NI_MAXSERV, NI_NUMERICHOST |
  // NI_NUMERICSERV)
  //     != 0) {
  //   return std::unexpected{std::errc(errno)};
  // }
  return std::make_tuple(Socket(io::NativeDevice{tmpfd}), addr);
}

static inline coro::Task<std::expected<int, std::errc>> connect(addrinfo *ai,
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

decltype(auto) connect(const Endpoint &ep, std::shared_ptr<Poller> poller) {
  return connect(ep.raw(), poller).transform([](auto &&exp) {
    exp.transform([](int fd) { return Socket(fd); });
  });
}
coro::Task<ConnectResult> connect(const EndpointSet &eps, std::shared_ptr<Poller> poller) {
  for (auto &ep : eps) {
    auto res = co_await connect(ep.raw(), poller);
    if (res) {
      co_return Socket(*res);
    }
  }
  co_return std::unexpected{std::errc{errno}};
}

static inline std::expected<int, std::errc> bind(addrinfo *ai) {
  int tmpfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  if (tmpfd == -1) [[unlikely]] {
    return std::unexpected{std::errc{errno}};
  }
  LOG5("Created fd: {}", tmpfd);
  if (auto snb_res = set_blocking<false>(tmpfd); !snb_res) [[unlikely]] {
    close(tmpfd);
    return std::unexpected{snb_res.error()};
  }
  LOG5("Set non-blocking to fd: {}", tmpfd);
  int opt = 1;
  if (setsockopt(tmpfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) [[unlikely]] {
    close(tmpfd);
    return std::unexpected{std::errc{errno}};
  }
  LOG5("Set reuse addr to fd: {}", tmpfd);
  if (bind(tmpfd, ai->ai_addr, ai->ai_addrlen) == -1) [[unlikely]] {
    close(tmpfd);
    return std::unexpected{std::errc{errno}};
  }
  return tmpfd;
}
BindResult bind(const Endpoint &ep) {
  return bind(ep.raw()).transform([](int fd) { return Socket(fd); });
}
BindResult bind(const EndpointSet &eps) {
  for (auto &ep : eps) {
    auto subres = bind(ep.raw());
    if (subres) {
      return Socket{*subres};
    }
  }
  return std::unexpected{std::errc{errno}};
}

std::expected<void, std::errc> listen(Socket &skt, int max_connections) {
  if (::listen(skt.raw(), max_connections) == -1) {
    return std::unexpected{std::errc{errno}};
  }
  return {};
}

SYS_NET_NE
