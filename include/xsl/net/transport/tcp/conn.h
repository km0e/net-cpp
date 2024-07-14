#pragma once
#ifndef XSL_NET_TRANSPORT_TCP_CONN
#  define XSL_NET_TRANSPORT_TCP_CONN
#  include "xsl/logctl.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/sync.h"
#  include "xsl/wheel.h"

#  include <sys/timerfd.h>

#  include <cstdint>
#  include <memory>

TCP_NB
using namespace sync;
enum class HandleHint {
  NONE = 0,
  // Hint that the param data is a pointer to a string that should be sent
  WRITE = 2,
};

enum class HandleState {
  NONE = 0,
  // active close
  CLOSE = 1,
  // hint that the connection should be kept alive
  KEEP_ALIVE = 2,
  // zero size read
  ZERO_SIZE_READ = 3,
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

template <class T, class H>
concept TcpHandlerGeneratorLike = TcpHandlerLike<H> && requires(T t, H h, int fd) {
  { t(fd) } -> std::same_as<std::unique_ptr<H>>;
};

class TcpHandler {
public:
  virtual ~TcpHandler() {}
  virtual HandleState send(int fd) = 0;
  virtual HandleState recv(int fd) = 0;
  virtual void close(int fd) = 0;
  virtual HandleState other(int fd, IOM_EVENTS events) = 0;
};
class TcpConnLimit {
public:
  int recv_timeout;
  uint8_t keep_alive_timeout_count;
  uint8_t zero_size_read_count;
};

class TcpConnFlag {
public:
  TcpConnFlag()
      : close(false),
        timeout(false),
        keep_alive(false),
        keep_alive_count(0),
        zero_size_read_count(0) {}
  bool close;
  bool timeout;
  bool keep_alive;
  uint8_t keep_alive_count;
  uint8_t zero_size_read_count;
};
template <TcpHandlerLike H>
class TcpConn {
public:
  TcpConn(int fd, const TcpConnLimit& limit, std::unique_ptr<H>&& handler)
      : fd(fd), events(IOM_EVENTS::IN), handler(move(handler)), flags(), limit(limit) {}

  TcpConn(TcpConn&&) = delete;
  TcpConn(const TcpConn&) = delete;
  ~TcpConn() {
    if (this->fd != -1) {
      close(this->fd);
    }
  }
  PollHandleHint operator()(int fd, IOM_EVENTS events) {
    this->flags.timeout = false;
    this->flags.keep_alive_count = 0;
    PollHandleHint hint{};
    if ((events & IOM_EVENTS::IN) == IOM_EVENTS::IN) {
      auto state = handler->recv(fd);
      hint = map_hint(state);
      events &= ~IOM_EVENTS::IN;
    }
    if ((events & IOM_EVENTS::OUT) == IOM_EVENTS::OUT) {
      auto state = handler->send(fd);
      hint = map_hint(state);
      events &= ~IOM_EVENTS::OUT;
    }
    if ((events & IOM_EVENTS::RDHUP) == IOM_EVENTS::RDHUP) {
      auto state = handler->recv(fd);
      hint = map_hint(state);
      if (!this->flags.close) {
        handler->close(fd);
        this->flags.close = true;
      }
      hint = {PollHandleHintTag::DELETE};
      events &= ~IOM_EVENTS::RDHUP;
    } else if ((events & IOM_EVENTS::HUP) == IOM_EVENTS::HUP) {
      handler->close(fd);
      this->flags.close = true;
      hint = {PollHandleHintTag::DELETE};
      events &= ~IOM_EVENTS::HUP;
    }
    if (events != IOM_EVENTS::NONE) {
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
      case HandleState::ZERO_SIZE_READ:
        this->flags.zero_size_read_count++;
        if (this->flags.zero_size_read_count == limit.zero_size_read_count) {
          this->handler->close(this->fd);
          this->flags.close = true;
          return {PollHandleHintTag::DELETE};
        }
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
  const TcpConnLimit& limit;
};
class TcpConnManagerConfig {
public:
  TcpConnManagerConfig(std::shared_ptr<Poller> poller)
      : poller(poller),
        recv_timeout(RECV_TIMEOUT),
        keep_alive_timeout_count(KEEP_ALIVE_TIMEOUT_COUNT),
        zero_size_read_count(ZERO_SIZE_READ_COUNT) {}
  std::shared_ptr<Poller> poller;
  int recv_timeout = RECV_TIMEOUT;
  uint8_t keep_alive_timeout_count = KEEP_ALIVE_TIMEOUT_COUNT;
  uint8_t zero_size_read_count = ZERO_SIZE_READ_COUNT;
};
template <TcpHandlerLike H>
class TcpConnManager {
public:
  TcpConnManager(TcpConnManagerConfig config)
      : poller(config.poller),
        limit(config.recv_timeout, config.keep_alive_timeout_count, config.zero_size_read_count),
        handlers(),
        timer_cnt() {
    this->setup_timer();
  }
  TcpConnManager(TcpConnManager&&) = delete;
  TcpConnManager(const TcpConnManager&) = delete;
  TcpConnManager& operator=(TcpConnManager&&) = delete;
  TcpConnManager& operator=(const TcpConnManager&) = delete;
  ~TcpConnManager() {
    for (auto& [_, handler] : *handlers.lock()) {
      this->poller->remove(handler->fd);
    }
  }
  void add(int fd, std::unique_ptr<H>&& handler) {
    this->handlers.lock()->emplace(fd, poll_add_unique<TcpConn<H>>(this->poller, fd, IOM_EVENTS::IN,
                                                                   fd, limit, std::move(handler)));
  }
  void remove(int fd) { handlers.lock()->erase(fd); }
  void clear() { handlers.lock()->clear(); }
  PollHandleHint operator()([[maybe_unused]] int fd, [[maybe_unused]] IOM_EVENTS events) {
    uint64_t exp;
    ssize_t s = read(fd, &exp, sizeof(uint64_t));
    if (s == -1) {
      ERROR("Failed to read timerfd, error: {}", strerror(errno));
      return {PollHandleHintTag::NONE};
    }
    std::vector<int> timeout, closed;
    {
      auto guard = handlers.lock();
      for (auto& [fd, conn] : *guard) {
        if (conn->flags.close) {
          closed.push_back(fd);
          continue;
        }
        if (conn->flags.keep_alive) {
          conn->flags.keep_alive_count++;
          if (conn->flags.keep_alive_count == limit.keep_alive_timeout_count) {
            timeout.push_back(fd);
            conn->handler->close(fd);
          }
          continue;
        }
        if (conn->flags.timeout) {
          timeout.push_back(fd);
          conn->handler->close(fd);
        } else {
          conn->flags.timeout = true;
        }
      }
      for (auto fd : closed) {
        guard->erase(fd);
        this->poller->remove(fd);
      }
      for (auto fd : timeout) {
        guard->erase(fd);
        this->poller->remove(fd);
      }
    }
    return {PollHandleHintTag::NONE};
  }

private:
  std::shared_ptr<Poller> poller;
  TcpConnLimit limit;

  ShrdRes<std::unordered_map<int, std::unique_ptr<TcpConn<H>>>> handlers;

  // timer cnt
  int timer_cnt;
  void setup_timer() {
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timer_fd == -1) {
      ERROR("Failed to create timerfd, error: {}", strerror(errno));
      return;
    }
    this->poller->add(timer_fd, IOM_EVENTS::IN,
                      [this](int fd, IOM_EVENTS events) { return (*this)(fd, events); });
    struct itimerspec new_value;
    new_value.it_value.tv_sec = limit.recv_timeout / 1000;
    new_value.it_value.tv_nsec = (limit.recv_timeout % 1000) * 1000000;
    new_value.it_interval.tv_sec = limit.recv_timeout / 1000;
    new_value.it_interval.tv_nsec = (limit.recv_timeout % 1000) * 1000000;
    if (timerfd_settime(timer_fd, 0, &new_value, nullptr) == -1) {
      ERROR("Failed to set timerfd, error: {}", strerror(errno));
    }
  }
};
TCP_NE

#endif
