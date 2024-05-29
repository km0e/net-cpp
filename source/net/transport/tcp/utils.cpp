#include "xsl/net/transport/tcp/def.h"
#include "xsl/net/transport/tcp/utils.h"
#include "xsl/utils.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <spdlog/spdlog.h>
#include <sys/types.h>

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

string_view to_string(SendError err) {
  switch (err) {
    case SendError::Unknown:
      return "Unknown";
    default:
      return "Unknown";
  }
}

string_view to_string(RecvError err) {
  switch (err) {
    case RecvError::Unknown:
      return "Unknown";
    case RecvError::Eof:
      return "Eof";
    default:
      return "Unknown";
  }
}

RecvResult recv(int fd) {
  SPDLOG_TRACE("start recv string");
  vector<string> data;
  char buf[MAX_SINGLE_RECV_SIZE];
  ssize_t n;
  do {
    n = ::recv(fd, buf, sizeof(buf), 0);
    SPDLOG_DEBUG("recv n: {}", n);
    if (n == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        if (data.empty()) {
          SPDLOG_DEBUG("recv eof");
          return {RecvError::Eof};
        }
        SPDLOG_DEBUG("recv over");
        break;
      } else {
        SPDLOG_ERROR("Failed to recv data, err : {}", strerror(errno));
        // TODO: handle recv error
        return {RecvError::Unknown};
      }
    } else if (n == 0) {
      SPDLOG_DEBUG("recv eof");
      return {RecvError::Eof};
      break;
    }
    data.emplace_back(buf, n);
  } while (n == sizeof(buf));
  return {accumulate(data.begin(), data.end(), string())};
}
SendResult send(int fd, string_view data) {
  ssize_t n = write(fd, data.data(), data.size());
  if (n == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return {0};
    } else {
      return {SendError::Unknown};
    }
  } else if (n == 0) {
    return {0};
  } else if (static_cast<size_t>(n) == data.size()) {
    return {static_cast<size_t>(n)};
  }
  data = data.substr(n);
  while (true) {
    ssize_t tmp = write(fd, data.data(), data.size());
    if (tmp == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return {static_cast<size_t>(n)};
      } else {
        return {SendError::Unknown};
      }
    } else if (tmp == 0) {
      return {static_cast<size_t>(n)};
    } else if (static_cast<size_t>(tmp) == data.size()) {
      return {static_cast<size_t>(n + tmp)};
    }
    data = data.substr(tmp);
    n += tmp;
  }
  return {SendError::Unknown};
}

TCP_NAMESPACE_END
