#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_CONN_H_
#  define _XSL_NET_TRANSPORT_TCP_CONN_H_
#  include "xsl/net/sync.h"
#  include "xsl/net/sync/poller.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/wheel.h"

#  include <spdlog/spdlog.h>
#  include <sys/timerfd.h>
TCP_NAMESPACE_BEGIN
enum class HandleHint {
  NONE = 0,
  // Hint that the param data is a pointer to a string that should be sent
  WRITE = 2,
};

enum class HandleState {
  NONE = 0,
  //
  CLOSE = 1,
  //
  KEEP_ALIVE = 2,
};

/**
 * @brief TcpHandlerLike concept
 * @tparam T type
 */
template <class T>
concept TcpHandlerLike = requires(T t, int fd, IOM_EVENTS events) {
  { t.send(fd) } -> std::same_as<HandleState>;
  { t.recv(fd) } -> std::same_as<HandleState>;
  { t.close(fd) } -> std::same_as<void>;
  { t.other(fd, events) } -> std::same_as<HandleState>;
};

class TcpHandler {
public:
  virtual ~TcpHandler() {}
  virtual HandleState send(int fd) = 0;
  virtual HandleState recv(int fd) = 0;
  virtual void close(int fd) = 0;
  virtual HandleState other(int fd, IOM_EVENTS events) = 0;
};

class TcpConnFlag {
public:
  TcpConnFlag() : close(false), timeout(false), keep_alive(false) {}
  bool close;
  bool timeout;
  bool keep_alive;
};
template <TcpHandlerLike H>
class TcpConn {
public:
  TcpConn(int fd, std::unique_ptr<H>&& handler)
      : fd(fd), events(IOM_EVENTS::IN), handler(move(handler)), flags() {}

  TcpConn(TcpConn&&) = delete;
  TcpConn(const TcpConn&) = delete;
  ~TcpConn() {
    if (this->fd != -1) {
      close(this->fd);
    }
  }
  PollHandleHint operator()(int fd, IOM_EVENTS events) {
    PollHandleHint hint{};
    if ((events & IOM_EVENTS::HUP) == IOM_EVENTS::HUP) {
      handler->close(fd);
      this->flags.close = true;
      hint = {PollHandleHintTag::DELETE};
    } else if ((events & IOM_EVENTS::IN) == IOM_EVENTS::IN) {
      this->flags.timeout = false;
      auto state = handler->recv(fd);
      hint = map_hint(state);
    } else if ((events & IOM_EVENTS::OUT) == IOM_EVENTS::OUT) {
      auto state = handler->send(fd);
      hint = map_hint(state);
    } else {
      auto state = handler->other(fd, events);
      hint = map_hint(state);
    }
    return hint;
  }
  PollHandleHint map_hint(HandleState state) {
    switch (state) {
      case HandleState::CLOSE:
        this->flags.close = true;
        return {PollHandleHintTag::DELETE};
      case HandleState::KEEP_ALIVE:
        this->flags.keep_alive = true;
        return {PollHandleHintTag::NONE};
      default:
        return {PollHandleHintTag::NONE};
    }
  }
  bool valid() { return this->fd != -1; }

  int fd;
  IOM_EVENTS events;
  std::unique_ptr<H> handler;
  TcpConnFlag flags;
};
class TcpConnManagerConfig {
public:
  TcpConnManagerConfig() : poller(nullptr) {}
  std::shared_ptr<Poller> poller;
  int recv_timeout = RECV_TIMEOUT;
};
template <TcpHandlerLike H>
class TcpConnManager {
public:
  TcpConnManager(TcpConnManagerConfig config) : config(config), handlers(), timer_cnt() {
    this->setup_timer();
  }
  TcpConnManager(TcpConnManager&&) = delete;
  TcpConnManager(const TcpConnManager&) = delete;
  TcpConnManager& operator=(TcpConnManager&&) = delete;
  TcpConnManager& operator=(const TcpConnManager&) = delete;
  ~TcpConnManager() {
    for (auto& [_, handler] : *handlers.lock()) {
      this->config.poller->remove(handler->fd);
    }
  }
  void add(int fd, std::unique_ptr<H>&& handler) {
    this->handlers.lock()->emplace(
        fd, poll_add_unique<TcpConn<H>>(this->config.poller, fd, IOM_EVENTS::IN, fd,
                                        std::move(handler)));
  }
  void remove(int fd) { handlers.lock()->erase(fd); }
  void clear() { handlers.lock()->clear(); }
  PollHandleHint operator()([[maybe_unused]] int fd, [[maybe_unused]] IOM_EVENTS events) {
    uint64_t exp;
    ssize_t s = read(fd, &exp, sizeof(uint64_t));
    if (s == -1) {
      SPDLOG_ERROR("Failed to read timerfd, error: {}", strerror(errno));
      return {PollHandleHintTag::NONE};
    }
    std::vector<int> timeout, closed;
    {
      auto guard = handlers.lock();
      for (auto& [fd, conn] : *guard) {
        if (conn->flags.close) {
          closed.push_back(fd);
        } else if (!conn->flags.keep_alive) {
          if (conn->flags.timeout) {
            timeout.push_back(fd);
            conn->handler->close(fd);
          } else {
            conn->flags.timeout = true;
          }
        }
      }
      for (auto fd : closed) {
        guard->erase(fd);
        this->config.poller->remove(fd);
      }
      for (auto fd : timeout) {
        guard->erase(fd);
        this->config.poller->remove(fd);
      }
    }
    // this->timer_cnt++;
    // if (this->timer_cnt == RECV_RESET_COUNT) {
    //   this->timer_cnt = 0;
    //   this->reset_timer(fd);
    // }
    return {PollHandleHintTag::NONE};
  }

private:
  TcpConnManagerConfig config;
  ShareContainer<std::unordered_map<int, std::unique_ptr<TcpConn<H>>>> handlers;

  // timer cnt
  int timer_cnt;
  void setup_timer() {
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timer_fd == -1) {
      SPDLOG_ERROR("Failed to create timerfd, error: {}", strerror(errno));
      return;
    }
    this->config.poller->add(timer_fd, IOM_EVENTS::IN,
                             [this](int fd, IOM_EVENTS events) { return (*this)(fd, events); });
    struct itimerspec new_value;
    new_value.it_value.tv_sec = config.recv_timeout / 1000;
    new_value.it_value.tv_nsec = (config.recv_timeout % 1000) * 1000000;
    new_value.it_interval.tv_sec = config.recv_timeout / 1000;
    new_value.it_interval.tv_nsec = (config.recv_timeout % 1000) * 1000000;
    SPDLOG_TRACE("tcp conn manager reset timer to {}:{} {}:{}", new_value.it_value.tv_sec,
                 new_value.it_value.tv_nsec, new_value.it_interval.tv_sec,
                 new_value.it_interval.tv_nsec);
    if (timerfd_settime(timer_fd, 0, &new_value, nullptr) == -1) {
      SPDLOG_ERROR("Failed to set timerfd, error: {}", strerror(errno));
    }
  }
};
TCP_NAMESPACE_END

#endif
