#include "xsl/logctl.h"
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
SYS_NET_NB

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
