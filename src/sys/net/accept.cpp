#include "xsl/logctl.h"
#include "xsl/sys/net/accept.h"
#include "xsl/sys/net/def.h"
#include "xsl/sys/net/socket.h"

#include <netdb.h>
#include <sys/io.h>
#include <sys/raw.h>
#include <sys/socket.h>

#include <expected>
#include <system_error>
SYS_NET_NB

AcceptResult accept(Socket &socket, SockAddr *addr) {
  auto [sockaddr, addrlen] = addr->raw();
  int tmpfd = ::accept4(socket.raw(), sockaddr, addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
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
  return Socket(io::NativeDevice{tmpfd});
}

SYS_NET_NE
