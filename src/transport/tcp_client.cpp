
#include <netdb.h>
#include <spdlog/spdlog.h>
#include <xsl/config.h>
#include <xsl/transport/tcp_client.h>
#include <xsl/transport/transport.h>

#include <cstring>
XSL_NAMESPACE_BEGIN
TRANSPORT_NAMESPACE_BEGIN
TcpClient::TcpClient() {
}
int TcpClient::connect(const char *host, const char *port) {
  addrinfo hints;
  addrinfo *result;
  int client_fd = -1;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  int res = getaddrinfo(host, port, &hints, &result);
  if(res != 0) {
    spdlog::warn("Failed to get address info for {}:{}", host, port);
    return -1;
  }
  addrinfo *rp;
  for(rp = result; rp != nullptr; rp = rp->ai_next) {
    int tmp_client_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if(tmp_client_fd == -1) {
      continue;
    }
    if(::connect(tmp_client_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
      client_fd = tmp_client_fd;
      break;
    }
    spdlog::warn("Connection failed: {}", strerror(errno));
    close(tmp_client_fd);
  }
  freeaddrinfo(result);
  if(rp == nullptr) {
    spdlog::warn("No address found for {}:{}", host, port);
    return -1;
  }
  return client_fd;
}
TRANSPORT_NAMESPACE_END
XSL_NAMESPACE_END