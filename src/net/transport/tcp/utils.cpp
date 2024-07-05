#include "xsl/coro/await.h"
#include "xsl/logctl.h"
#include "xsl/net/transport/tcp/def.h"
#include "xsl/net/transport/tcp/utils.h"
#include "xsl/utils.h"
#include "xsl/utils/fd.h"
#include "xsl/wheel/result.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>

#include <cerrno>
#include <cstdlib>
#include <functional>
#include <memory>
#include <numeric>
#include <system_error>

TCP_NAMESPACE_BEGIN
SockAddrV4View::SockAddrV4View(const char *sa4) : _ip(), _port() {
  std::string_view sa4_view(sa4);
  size_t pos = sa4_view.find(':');
  if (pos == std::string_view::npos) {
    _ip = sa4_view;
    _port = "";
  } else {
    _ip = sa4_view.substr(0, pos);
    _port = sa4_view.substr(pos + 1);
  }
}
SockAddrV4View::SockAddrV4View(std::string_view sa4) : _ip(), _port() {
  size_t pos = sa4.find(':');
  if (pos == std::string_view::npos) {
    _ip = sa4;
    _port = "";
  } else {
    _ip = sa4.substr(0, pos);
    _port = sa4.substr(pos + 1);
  }
}
SockAddrV4View::SockAddrV4View(const char *ip, const char *port) : _ip(ip), _port(port) {}
SockAddrV4View::SockAddrV4View(std::string_view ip, std::string_view port) : _ip(ip), _port(port) {}
bool SockAddrV4View::operator==(const SockAddrV4View &rhs) const {
  return _ip == rhs._ip && _port == rhs._port;
}

const char *SockAddrV4View::ip() const { return _ip.data(); }
const char *SockAddrV4View::port() const { return _port.data(); }
std::string SockAddrV4View::to_string() const { return format("{}:{}", _ip, _port); }
SockAddrV4::SockAddrV4(int fd) : _ip(), _port() {
  sockaddr addr;
  socklen_t len = sizeof(addr);
  getpeername(fd, &addr, &len);
  sockaddr_in *addr_in = (sockaddr_in *)&addr;
  _ip = inet_ntoa(addr_in->sin_addr);
  _port = std::to_string(ntohs(addr_in->sin_port));
}
SockAddrV4::SockAddrV4(const char *ip, const char *port) : _ip(ip), _port(port) {}
SockAddrV4::SockAddrV4(std::string_view ip, std::string_view port) : _ip(ip), _port(port) {}
SockAddrV4::SockAddrV4(SockAddrV4View sa4) : _ip(sa4._ip), _port(sa4._port) {}
SockAddrV4View SockAddrV4::view() const { return SockAddrV4View(_ip, _port); }

bool SockAddrV4::operator==(const SockAddrV4View &rhs) const {
  return _ip == rhs._ip && _port == rhs._port;
}

bool SockAddrV4::operator==(const SockAddrV4 &rhs) const {
  return _ip == rhs._ip && _port == rhs._port;
}
std::string SockAddrV4::to_string() const { return format("{}:{}", _ip, _port); }

coro::Task<ConnectResult> connect(const AddrInfo &ai, std::shared_ptr<Poller> poller) {
  ConnectResult res = {std::error_code()};
  for (addrinfo *rp = ai.info; rp != nullptr; rp = rp->ai_next) {
    int tmpfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (tmpfd == -1) {
      continue;
    }
    DEBUG("Created fd: {}", tmpfd);
    if (!utils::set_non_blocking(tmpfd)) {
      res = {std::make_error_code(std::errc{errno})};
      close(tmpfd);
      continue;
    }
    DEBUG("Set non-blocking to fd: {}", tmpfd);
    int ec = ::connect(tmpfd, rp->ai_addr, rp->ai_addrlen);
    if (ec == 0) {
      res = {Socket(tmpfd)};
      DEBUG("Connected to fd: {}", tmpfd);
    } else {
      WARNING("Failed to connect to fd: {}", tmpfd);
      auto func = [tmpfd, &poller](auto set_result) {
        poller->add(tmpfd, IOM_EVENTS::OUT, [set_result](int fd, IOM_EVENTS events) {
          if (!!(events & IOM_EVENTS::OUT)) {
            DEBUG("Fd: {} is writable", fd);
            int opt;
            socklen_t len = sizeof(opt);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &opt, &len) == -1) {
              ERROR("Failed to getsockopt: {}", strerror(errno));
              set_result({errno});
              return PollHandleHintTag::DELETE;
            }
            if (opt != 0) {
              ERROR("Failed to connect: {}", strerror(opt));
              set_result({opt});
              return PollHandleHintTag::DELETE;
            }
            DEBUG("Connected to fd: {}", fd);
            set_result(Socket(fd));
            return PollHandleHintTag::DELETE;
          } else if (!events) {
            ERROR("Timeout");
            set_result({ETIMEDOUT});
            return PollHandleHintTag::DELETE;
          }
          return PollHandleHintTag::DELETE;
        });
      };
      res = co_await coro::CallbackAwaiter<ConnectResult>(func);
      if (res.is_ok()) {
        break;
      }
    }
    close(tmpfd);
  }
  co_return res;
}

int new_tcp_client(const char *ip, const char *port, TcpClientSockConfig config) {
  DEBUG("Connecting to {}:{}", ip, port);
  addrinfo hints;
  addrinfo *result;
  int client_fd = -1;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  int res = getaddrinfo(ip, port, &hints, &result);
  if (res != 0) {
    WARNING("getaddrinfo failed: {}", gai_strerror(res));
    return -1;
  }
  addrinfo *rp;
  for (rp = result; rp != nullptr; rp = rp->ai_next) {
    int tmp_client_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (tmp_client_fd == -1) {
      continue;
    }
    if (::connect(tmp_client_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
      if (config.keep_alive && !set_keep_alive(tmp_client_fd, true)) {
        WARNING("Failed to set keep alive");
        close(tmp_client_fd);
        continue;
      }
      if (config.non_blocking && !set_non_blocking(tmp_client_fd)) {
        WARNING("Failed to set non-blocking");
        close(tmp_client_fd);
        continue;
      }
      client_fd = tmp_client_fd;
      break;
    }
    WARNING("Failed to connect to {}:{}", ip, port);
    close(tmp_client_fd);
  }
  freeaddrinfo(result);
  if (rp == nullptr) {
    WARNING("Failed to connect to {}:{}", ip, port);
    return -1;
  }
  return client_fd;
}
int new_tcp_client(const SockAddrV4 &sa4, TcpClientSockConfig config) {
  return new_tcp_client(sa4._ip.data(), sa4._port.data(), config);
}

int new_tcp_server(const char *ip, int port, TcpServerSockConfig config) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  DEBUG("Server fd: {}", server_fd);
  if (server_fd == -1) {
    ERROR("Failed to create socket");
    return -1;
  }
  if (config.keep_alive && !set_keep_alive(server_fd, true)) {
    close(server_fd);
    ERROR("Failed to set keep alive");
    return -1;
  }
  if (config.non_blocking && !set_non_blocking(server_fd)) {
    close(server_fd);
    ERROR("Failed to set non-blocking");
    return -1;
  }
  if (config.reuse_addr) {
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
      close(server_fd);
      ERROR("Failed to set reuse addr");
      return -1;
    }
  }
  sockaddr addr;
  sockaddr_in *addr_in = (sockaddr_in *)&addr;
  addr_in->sin_family = AF_INET;
  addr_in->sin_port = htons(port);
  addr_in->sin_addr.s_addr = inet_addr(ip);
  if (bind(server_fd, &addr, sizeof(addr)) == -1) {
    close(server_fd);
    ERROR("Failed to bind on {}:{}", ip, port);
    return -1;
  }
  if (listen(server_fd, config.max_connections) == -1) {
    close(server_fd);
    ERROR("Failed to listen on {}:{}", ip, port);
    return -1;
  }
  return server_fd;
}
int new_tcp_server(const SockAddrV4 &sa4, TcpServerSockConfig config) {
  return new_tcp_server(sa4._ip.data(), strtol(sa4._port.data(), nullptr, 10), config);
}

bool set_keep_alive(int fd, bool keep_alive) {
  int opt = keep_alive ? 1 : 0;
  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt))) {
    ERROR("Failed to set keep alive");
    return false;
  }
  return true;
}

std::string_view to_string_view(SendError err) {
  switch (err) {
    case SendError::Unknown:
      return "Unknown";
    default:
      return "Unknown";
  }
}

std::string_view to_string_view(RecvError err) {
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
  TRACE("start recv string");
  std::vector<std::string> data;
  char buf[MAX_SINGLE_RECV_SIZE];
  ssize_t n;
  do {
    n = ::recv(fd, buf, sizeof(buf), 0);
    DEBUG("recv n: {}", n);
    if (n == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        if (data.empty()) {
          DEBUG("recv eof");
          return {RecvError::Eof};
        }
        DEBUG("recv over");
        break;
      } else {
        ERROR("Failed to recv data, err : {}", strerror(errno));
        // TODO: handle recv error
        return {RecvError::Unknown};
      }
    } else if (n == 0) {
      DEBUG("recv eof");
      return {RecvError::Eof};
      break;
    }
    data.emplace_back(buf, n);
  } while (n == sizeof(buf));
  return {std::accumulate(data.begin(), data.end(), std::string())};
}
SendResult send(int fd, std::string_view data) {
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
