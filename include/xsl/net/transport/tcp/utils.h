#pragma once
#include "xsl/logctl.h"

#include <optional>
#ifndef _XSL_NET_TRANSPORT_UTILS_H_
#  define _XSL_NET_TRANSPORT_UTILS_H_
#  include "xsl/coro/task.h"
#  include "xsl/net/sync.h"
#  include "xsl/net/transport/resolve.h"
#  include "xsl/net/transport/tcp/def.h"

#  include <netdb.h>
#  include <sys/socket.h>

#  include <expected>
#  include <functional>
#  include <generator>
#  include <memory>
#  include <mutex>
#  include <queue>
#  include <system_error>
#  include <tuple>

TCP_NAMESPACE_BEGIN
class SockAddrV4View {
public:
  SockAddrV4View() = default;
  SockAddrV4View(const char *ip, const char *port);
  // eg: "0.0.0.1:80"
  SockAddrV4View(const char *sa4);
  SockAddrV4View(std::string_view sa4);
  SockAddrV4View(std::string_view ip, std::string_view port);
  bool operator==(const SockAddrV4View &rhs) const;
  const char *ip() const;
  const char *port() const;
  std::string to_string() const;
  std::string_view _ip;
  std::string_view _port;
};

class SockAddrV4 {
public:
  SockAddrV4() = default;
  SockAddrV4(int fd);
  SockAddrV4(const char *ip, const char *port);
  SockAddrV4(std::string_view ip, std::string_view port);
  SockAddrV4(SockAddrV4View sa4);
  SockAddrV4View view() const;
  bool operator==(const SockAddrV4View &rhs) const;
  bool operator==(const SockAddrV4 &rhs) const;
  std::string to_string() const;
  std::string _ip;
  std::string _port;
};

using ConnectResult = std::expected<Socket, std::errc>;

coro::Task<ConnectResult> connect(const AddrInfo &ai, std::shared_ptr<Poller> poller);

using BindResult = std::expected<Socket, std::errc>;

BindResult bind(const AddrInfo &ai);

using ListenResult = std::expected<void, std::errc>;

ListenResult listen(Socket &skt, int max_connections = 10);

using AcceptResult = std::expected<Socket, std::errc>;

class Acceptor {
  class AcceptorImpl {
  public:
    AcceptorImpl(Socket &&skt) : skt(std::move(skt)), cb(std::nullopt), mtx_(), events() {}
    PollHandleHintTag operator()(int fd, IOM_EVENTS events) {
      DEBUG("acceptor");
      this->mtx_.lock();
      this->events.push({fd, events});
      if (this->cb) {
        DEBUG("dispatch");
        (*this->cb)();
        this->cb.reset();
      } else {
        DEBUG("no cb");
        this->mtx_.unlock();
      }
      return PollHandleHintTag::NONE;
    }

    Socket skt;

    std::optional<std::function<void()>> cb;

    std::mutex mtx_;
    std::queue<std::tuple<int, IOM_EVENTS>> events;
  };
  bool await_ready() noexcept {
    DEBUG("");
    this->impl->mtx_.lock();
    return !this->impl->events.empty();
  }
  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    DEBUG("need to suspend");
    this->impl->cb = [handle]() { handle.promise().dispatch([handle]() { handle.resume(); }); };
    this->impl->mtx_.unlock();
  }
  ConnectResult await_resume() noexcept {
    DEBUG("return result");
    this->impl->mtx_.unlock();
    auto [fd, events] = this->impl->events.front();
    this->impl->events.pop();
    if (!!(events & IOM_EVENTS::IN)) {
      DEBUG("Fd: {} is readable", fd);
      sockaddr addr;
      socklen_t len = sizeof(addr);
      int new_fd = ::accept(fd, &addr, &len);
      if (new_fd == -1) {
        ERROR("Failed to accept: {}", strerror(errno));
        return {errno};
      }
      DEBUG("Accepted new fd: {}", new_fd);
      return {Socket(new_fd)};
    } else if (!events) {
      ERROR("Timeout");
      return {ETIMEDOUT};
    }
    return {ECONNREFUSED};
  }

public:
  Acceptor(Socket &&skt, std::shared_ptr<Poller> poller)
      : impl(std::make_unique<AcceptorImpl>(std::move(skt))) {
    poller->add(impl->skt.raw_fd(), IOM_EVENTS::IN,
                [impl = impl.get()](int fd, IOM_EVENTS events) { return (*impl)(fd, events); });
  }
  ~Acceptor() = default;

  coro::Task<AcceptResult> accept() {
    DEBUG("");
    co_return co_await *this;
  }

private:
  std::unique_ptr<AcceptorImpl> impl;
};

bool set_keep_alive(int fd, bool keep_alive = true);

const size_t MAX_SINGLE_RECV_SIZE = 1024;

enum class SendError {
  Unknown,
};

std::string_view to_string_view(SendError err);
using SendResult = std::expected<bool, SendError>;
enum class RecvError {
  Unknown,
  Eof,
  NoData,
};
std::string_view to_string_view(RecvError err);
using RecvResult = std::expected<std::string, RecvError>;
SendResult send(int fd, std::string_view data);
RecvResult recv(int fd);
TCP_NAMESPACE_END
#endif  // _XSL_NET_TRANSPORT_UTILS_H_
