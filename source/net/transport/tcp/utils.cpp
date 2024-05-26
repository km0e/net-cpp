#include "xsl/net/transport/tcp/def.h"
#include "xsl/net/transport/tcp/utils.h"
#include "xsl/utils.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <spdlog/spdlog.h>

TCP_NAMESPACE_BEGIN

int create_tcp_client(const char *ip, const char *port, TcpClientSockConfig config) {
  SPDLOG_DEBUG("Connecting to {}:{}", ip, port);
  addrinfo hints;
  addrinfo *result;
  int client_fd = -1;
  SPDLOG_DEBUG("getaddrinfo");
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  int res = getaddrinfo(ip, port, &hints, &result);
  if (res != 0) {
    SPDLOG_WARN("getaddrinfo failed: {}", gai_strerror(res));
    return -1;
  }
  SPDLOG_DEBUG("getaddrinfo success");
  addrinfo *rp;
  for (rp = result; rp != nullptr; rp = rp->ai_next) {
    int tmp_client_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (tmp_client_fd == -1) {
      continue;
    }
    if (::connect(tmp_client_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
      if (config.keep_alive && !set_keep_alive(tmp_client_fd, true)) {
        SPDLOG_WARN("Failed to set keep alive");
        close(tmp_client_fd);
        continue;
      }
      if (config.non_blocking && !set_non_blocking(tmp_client_fd)) {
        SPDLOG_WARN("Failed to set non-blocking");
        close(tmp_client_fd);
        continue;
      }
      client_fd = tmp_client_fd;
      break;
    }
    SPDLOG_WARN("Failed to connect to {}:{}", ip, port);
    close(tmp_client_fd);
  }
  SPDLOG_DEBUG("Free addrinfo");
  freeaddrinfo(result);
  if (rp == nullptr) {
    SPDLOG_WARN("Failed to connect to {}:{}", ip, port);
    return -1;
  }
  SPDLOG_DEBUG("Connected to {}:{}", ip, port);
  return client_fd;
}

int create_tcp_server(const char *ip, int port, TcpServerSockConfig config) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  SPDLOG_DEBUG("Server fd: {}", server_fd);
  if (server_fd == -1) {
    SPDLOG_ERROR("Failed to create socket");
    return -1;
  }
  if (config.keep_alive && !set_keep_alive(server_fd, true)) {
    close(server_fd);
    SPDLOG_ERROR("Failed to set keep alive");
    return -1;
  }
  if (config.non_blocking && !set_non_blocking(server_fd)) {
    close(server_fd);
    SPDLOG_ERROR("Failed to set non-blocking");
    return -1;
  }
  sockaddr addr;
  sockaddr_in *addr_in = (sockaddr_in *)&addr;
  addr_in->sin_family = AF_INET;
  addr_in->sin_port = htons(port);
  addr_in->sin_addr.s_addr = inet_addr(ip);
  if (bind(server_fd, &addr, sizeof(addr)) == -1) {
    close(server_fd);
    SPDLOG_ERROR("Failed to bind on {}:{}", ip, port);
    return -1;
  }
  if (listen(server_fd, config.max_connections) == -1) {
    close(server_fd);
    SPDLOG_ERROR("Failed to listen on {}:{}", ip, port);
    return -1;
  }
  return server_fd;
}
bool set_keep_alive(int fd, bool keep_alive) {
  int opt = keep_alive ? 1 : 0;
  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt))) {
    SPDLOG_ERROR("Failed to set keep alive");
    return false;
  }
  return true;
}

TCP_NAMESPACE_END
