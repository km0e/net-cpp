#include "xsl/coro/await.h"
#include "xsl/logctl.h"
#include "xsl/net/transport/tcp/def.h"
#include "xsl/net/transport/tcp/utils.h"
#include "xsl/sync.h"
#include "xsl/utils/fd.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <memory>
#include <system_error>

TCP_NB

class EventAwaiter : public coro::CallbackAwaiter<std::tuple<int, IOM_EVENTS>, ConnectResult> {
public:
  using Base = coro::CallbackAwaiter<std::tuple<int, IOM_EVENTS>, ConnectResult>;

  using Base::Base;

  ConnectResult await_resume() noexcept {
    DEBUG("return result");
    auto [fd, events] = *_ntf;
    if (!!(events & IOM_EVENTS::OUT)) {
      DEBUG("Fd: {} is writable", fd);
      int opt;
      socklen_t len = sizeof(opt);
      if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &opt, &len) == -1) {
        ERROR("Failed to getsockopt: {}", strerror(errno));
        return {errno};
      }
      if (opt != 0) {
        ERROR("Failed to connect: {}", strerror(opt));
        return {opt};
      }
      DEBUG("Connected to fd: {}", fd);
      return {Socket(fd)};
    } else if (!events) {
      ERROR("Timeout");
      return {ETIMEDOUT};
    }
    return {ECONNREFUSED};
  }

private:
  using Base::_ntf;
};

coro::Task<ConnectResult> connect(const AddrInfo &ai, std::shared_ptr<Poller> poller) {
  ConnectResult res = std::unexpected{std::errc()};
  for (auto &ai : ai) {
    int tmpfd = socket(ai.ai_family, ai.ai_socktype, ai.ai_protocol);
    if (tmpfd == -1) {
      continue;
    }
    DEBUG("Created fd: {}", tmpfd);
    if (!utils::set_non_blocking(tmpfd)) {
      res = std::unexpected{std::errc{errno}};
      close(tmpfd);
      continue;
    }
    DEBUG("Set non-blocking to fd: {}", tmpfd);
    int ec = ::connect(tmpfd, ai.ai_addr, ai.ai_addrlen);
    if (ec == 0) {
      res.emplace(tmpfd);
      DEBUG("Connected to fd: {}", tmpfd);
    } else {
      WARNING("Failed to connect to fd: {}", tmpfd);
      auto func = [tmpfd, &poller](auto set_result) {
        poller->add(tmpfd, IOM_EVENTS::OUT, [set_result](int fd, IOM_EVENTS events) {
          set_result({fd, events});
          return PollHandleHintTag::DELETE;
        });
      };
      res = co_await EventAwaiter(func);
      if (res.has_value()) {
        break;
      }
    }
    close(tmpfd);
  }
  co_return res;
}

BindResult bind(const AddrInfo &ai) {
  BindResult res = std::unexpected{std::errc()};
  for (auto &ai : ai) {
    int tmpfd = socket(ai.ai_family, ai.ai_socktype, ai.ai_protocol);
    if (tmpfd == -1) {
      continue;
    }
    DEBUG("Created fd: {}", tmpfd);
    if (!utils::set_non_blocking(tmpfd)) {
      res = std::unexpected{std::errc{errno}};
      close(tmpfd);
      continue;
    }
    DEBUG("Set non-blocking to fd: {}", tmpfd);
    int opt = 1;
    if (setsockopt(tmpfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
      res = std::unexpected{std::errc{errno}};
      close(tmpfd);
      continue;
    }
    DEBUG("Set reuse addr to fd: {}", tmpfd);
    if (bind(tmpfd, ai.ai_addr, ai.ai_addrlen) == -1) {
      res = std::unexpected{std::errc{errno}};
      close(tmpfd);
      continue;
    }
    res.emplace(tmpfd);
    DEBUG("Bind to fd: {}", tmpfd);
    break;
  }
  return res;
}

std::expected<void, std::error_condition> listen(Socket &skt, int max_connections) {
  if (::listen(skt.raw_fd(), max_connections) == -1) {
    return std::unexpected{std::errc{errno}};
  }
  return {};
}

bool set_keep_alive(int fd, bool keep_alive) {
  int opt = keep_alive ? 1 : 0;
  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt))) {
    ERROR("Failed to set keep alive");
    return false;
  }
  return true;
}

// std::string_view to_string_view(SendError err) {
//   switch (err) {
//     case SendError::Unknown:
//       return "Unknown";
//     default:
//       return "Unknown";
//   }
// }

// std::string_view to_string_view(RecvError err) {
//   switch (err) {
//     case RecvError::Unknown:
//       return "Unknown";
//     case RecvError::Eof:
//       return "Eof";
//     default:
//       return "Unknown";
//   }
// }

// RecvResult recv(int fd) {
//   TRACE("start recv string");
//   std::vector<std::string> data;
//   char buf[MAX_SINGLE_RECV_SIZE];
//   ssize_t n;
//   do {
//     n = ::recv(fd, buf, sizeof(buf), 0);
//     DEBUG("recv n: {}", n);
//     if (n == -1) {
//       if (errno == EAGAIN || errno == EWOULDBLOCK) {
//         if (data.empty()) {
//           DEBUG("recv eof");
//           return std::unexpected{RecvError::Eof};
//         }
//         DEBUG("recv over");
//         break;
//       } else {
//         ERROR("Failed to recv data, err : {}", strerror(errno));
//         // TODO: handle recv error
//         return std::unexpected{RecvError::Unknown};
//       }
//     } else if (n == 0) {
//       DEBUG("recv eof");
//       return std::unexpected{RecvError::Eof};
//       break;
//     }
//     data.emplace_back(buf, n);
//   } while (n == sizeof(buf));
//   return {std::accumulate(data.begin(), data.end(), std::string())};
// }
// SendResult send(int fd, std::string_view data) {
//   ssize_t n = write(fd, data.data(), data.size());
//   if (n == -1) {
//     if (errno == EAGAIN || errno == EWOULDBLOCK) {
//       return {true};
//     } else {
//       return std::unexpected{SendError::Unknown};
//     }
//   } else if (n == 0) {
//     return {0};
//   } else if (static_cast<size_t>(n) == data.size()) {
//     return {static_cast<size_t>(n)};
//   }
//   data = data.substr(n);
//   while (true) {
//     ssize_t tmp = write(fd, data.data(), data.size());
//     if (tmp == -1) {
//       if (errno == EAGAIN || errno == EWOULDBLOCK) {
//         return {static_cast<size_t>(n)};
//       } else {
//         return std::unexpected{SendError::Unknown};
//       }
//     } else if (tmp == 0) {
//       return {static_cast<size_t>(n)};
//     } else if (static_cast<size_t>(tmp) == data.size()) {
//       return {static_cast<size_t>(n + tmp)};
//     }
//     data = data.substr(tmp);
//     n += tmp;
//   }
//   return std::unexpected{SendError::Unknown};
// }

TCP_NE
