#include "xsl/logctl.h"

#include <xsl/net/transport/tcp/def.h>

#include <utility>
TCP_NAMESPACE_BEGIN
Socket::Socket(int sfd) noexcept : sfd(sfd) {}
Socket::Socket(Socket &&rhs) noexcept : sfd(std::exchange(rhs.sfd, 0)) {}
Socket &Socket::operator=(Socket &&rhs) noexcept {
  sfd = std::exchange(rhs.sfd, 0);
  return *this;
}
Socket::~Socket() {
  DEBUG("close socket {}", sfd);
  if (sfd) close(sfd);
}
int Socket::raw_fd() const { return sfd; }

TCP_NAMESPACE_END
