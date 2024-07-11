#include "xsl/logctl.h"
#include "xsl/sys/net/def.h"
#include "xsl/sys/net/socket.h"
#include "xsl/utils.h"

#include <netdb.h>
#include <sys/socket.h>

#include <expected>
#include <system_error>
#include <tuple>
#include <utility>
SYS_NET_NB

Socket::Socket(int sfd) noexcept : sfd(sfd) {}
Socket::Socket(Socket &&rhs) noexcept : sfd(std::exchange(rhs.sfd, -1)) {}
Socket &Socket::operator=(Socket &&rhs) noexcept {
  sfd = std::exchange(rhs.sfd, -1);
  return *this;
}
Socket::~Socket() {
  if (sfd > 0) {
    close(sfd);
    DEBUG("close socket {}", sfd);
  }
}
int Socket::raw_fd() const { return sfd; }
std::expected<void, std::error_condition> Socket::set_blocking(bool blocking) {
  if (set_non_blocking(this->raw_fd(), !blocking)) {
    return {};
  }
  return std::unexpected{std::errc(errno)};
}

AcceptResult accept(Socket &skt) {
  sockaddr addr;
  socklen_t addrlen = sizeof(addr);
  int fd = ::accept4(skt.raw_fd(), &addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (fd < 0) {
    return AcceptResult{std::unexpect, std::errc(errno)};
  }
  char ip[NI_MAXHOST], port[NI_MAXSERV];
  if (getnameinfo(&addr, addrlen, ip, NI_MAXHOST, port, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV)
      != 0) {
    return AcceptResult{std::unexpect, std::errc(errno)};
  }
  return std::make_tuple(Socket(fd), IpAddr(ip, port));
}

SYS_NET_NE
