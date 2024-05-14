#include "xsl/net/transport/tcp/def.h"
#include "xsl/net/transport/tcp/utils.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <spdlog/spdlog.h>

#include <cstring>
TCP_NAMESPACE_BEGIN

int create_tcp_client(const char *ip, const char *port) {
  SPDLOG_TRACE("[TcpClient::connect] Connecting to {}:{}", ip, port);
  addrinfo hints;
  addrinfo *result;
  int client_fd = -1;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  int res = getaddrinfo(ip, port, &hints, &result);
  if (res != 0) {
    SPDLOG_WARN("[TcpClient::connect] getaddrinfo failed: {}", gai_strerror(res));
    return -1;
  }
  addrinfo *rp;
  for (rp = result; rp != nullptr; rp = rp->ai_next) {
    int tmp_client_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (tmp_client_fd == -1) {
      continue;
    }
    if (::connect(tmp_client_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
      client_fd = tmp_client_fd;
      break;
    }
    SPDLOG_WARN("[TcpClient::connect] Failed to connect to {}:{}", ip, port);
    close(tmp_client_fd);
  }
  freeaddrinfo(result);
  if (rp == nullptr) {
    SPDLOG_WARN("[TcpClient::connect] Failed to connect to {}:{}", ip, port);
    return -1;
  }
  return client_fd;
}

int create_tcp_server(const char *ip, int port, TcpConfig config) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  SPDLOG_DEBUG("Server fd: {}", server_fd);
  if (server_fd == -1) {
    SPDLOG_ERROR("Failed to create socket");
    return -1;
  }
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    close(server_fd);
    SPDLOG_ERROR("Failed to set socket options");
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

int read(int fd, wheel::string &data) {
  SPDLOG_TRACE("[read] Reading from fd: {}", fd);
  data.clear();
  data.reserve(1024);
  char buf[1024];
  ssize_t n;
  while ((n = recv(fd, buf, sizeof(buf), 0)) > 0) {
    data.append(buf, n);
  }
  SPDLOG_DEBUG("[read] data size: {}", data.size());
  if (n == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      SPDLOG_DEBUG("[read] recv over");
      return 0;
    } else {
      SPDLOG_ERROR("[read] Failed to recv data");
      return -1;
    }
  }
  if (n == 0) {
    SPDLOG_DEBUG("[read] recv over");
  }
  return 0;
}
int write(int fd, const wheel::string &data) {
  SPDLOG_TRACE("[write] Writing to fd: {}", fd);
  ssize_t n = send(fd, data.c_str(), data.size(), 0);
  if (n == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      SPDLOG_DEBUG("[write] send over");
    } else {
      SPDLOG_ERROR("[write] Failed to send data");
    }
    return 0;
  }
  return n;
}

TCP_NAMESPACE_END
