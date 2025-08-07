/**
 * @file context.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Context for I/O operations
 * @version 0.1.1
 * @date 2025-06-02
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_IO_CONTEXT
#  define XSL_IO_CONTEXT
#  include <sys/epoll.h>
#  include <xsl/io/def.h>
#  include <xsl/sync.h>

#  include <csignal>
#  include <memory>
XSL_IO_NB

const int TIMEOUT = 100;
#  define USE_EPOLL
#  ifdef USE_EPOLL
enum class IOM_EVENTS : std::uint32_t {
  NONE = 0,
  IN = EPOLL_EVENTS::EPOLLIN,
  PRI = EPOLL_EVENTS::EPOLLPRI,
  OUT = EPOLL_EVENTS::EPOLLOUT,
  RDNORM = EPOLL_EVENTS::EPOLLRDNORM,
  RDBAND = EPOLL_EVENTS::EPOLLRDBAND,
  WRNORM = EPOLL_EVENTS::EPOLLWRNORM,
  WRBAND = EPOLL_EVENTS::EPOLLWRBAND,
  MSG = EPOLL_EVENTS::EPOLLMSG,
  ERR = EPOLL_EVENTS::EPOLLERR,
  HUP = EPOLL_EVENTS::EPOLLHUP,
  RDHUP = EPOLL_EVENTS::EPOLLRDHUP,
  EXCLUSIVE = EPOLL_EVENTS::EPOLLEXCLUSIVE,
  WAKEUP = EPOLL_EVENTS::EPOLLWAKEUP,
  ONESHOT = EPOLL_EVENTS::EPOLLONESHOT,
  ET = EPOLL_EVENTS::EPOLLET,
};

constexpr IOM_EVENTS operator|(IOM_EVENTS a, IOM_EVENTS b) {
  return static_cast<IOM_EVENTS>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
constexpr IOM_EVENTS& operator|=(IOM_EVENTS& a, IOM_EVENTS b);
constexpr IOM_EVENTS operator&(IOM_EVENTS a, IOM_EVENTS b) {
  return static_cast<IOM_EVENTS>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
constexpr IOM_EVENTS& operator&=(IOM_EVENTS& a, IOM_EVENTS b);
constexpr IOM_EVENTS operator~(IOM_EVENTS a);
constexpr bool operator!(IOM_EVENTS a) { return a == IOM_EVENTS::NONE; }
constexpr std::string to_string(IOM_EVENTS events) {
  // format to binary
  return std::format("{:b}", static_cast<uint32_t>(events));
}
#  endif

enum class PollHandleHintTag : uint8_t {
  NONE = 0,
  MODIFY = 1,
  DELETE = 2,
};

constexpr std::string_view to_string_view(PollHandleHintTag tag) {
  switch (tag) {
    case PollHandleHintTag::NONE:
      return "NONE";
    case PollHandleHintTag::MODIFY:
      return "MODIFY";
    case PollHandleHintTag::DELETE:
      return "DELETE";
    default:
      return "UNKNOWN";
  }
}

class PollHandleHint {
public:
  PollHandleHintTag tag;
  union {
    IOM_EVENTS events;
  } data;
  constexpr PollHandleHint() : tag(PollHandleHintTag::NONE), data{IOM_EVENTS::NONE} {}
  constexpr PollHandleHint(PollHandleHintTag tag) : tag(tag), data{IOM_EVENTS::NONE} {}
  constexpr PollHandleHint(PollHandleHintTag tag, IOM_EVENTS events) : tag(tag), data{events} {}
};
template <class T>
concept Handler = requires(T t) {
  { t(0, IOM_EVENTS::NONE) } -> std::same_as<PollHandleHint>;
};

using PollHandler = std::move_only_function<PollHandleHint(int fd, IOM_EVENTS events)>;

struct DefaultPollTraits {
  static constexpr PollHandleHintTag poll_check(IOM_EVENTS events) {
    return ((!events) || !!(events & IOM_EVENTS::HUP)) ? PollHandleHintTag::DELETE
                                                       : PollHandleHintTag::NONE;
  }
};

template <class Storage>
struct PollHandlerTraits;

template <class Traits, class Storage>
class PollForCoro : public Storage {
public:
  using traits_type = PollHandlerTraits<Storage>;

  template <class... Args>
    requires std::constructible_from<Storage, Args...>
  constexpr PollForCoro(Traits, Args&&... args) : Storage(std::forward<Args>(args)...) {}

  constexpr PollHandleHint operator()(int, IOM_EVENTS events) {
    if (Traits::poll_check(events) == PollHandleHintTag::DELETE) {
      return PollHandleHintTag::DELETE;
    } else {
      traits_type::pubsub(*this)->publish([&events](IOM_EVENTS e) { return !!(events & e); });
      return PollHandleHintTag::NONE;
    }
  }
};

using HandleProxy = std::function<PollHandleHint(std::function<PollHandleHint()>&&)>;
class Context {
public:
  Context();
  Context(std::shared_ptr<HandleProxy>&& proxy);
  ~Context();
  constexpr bool valid() { return this->fd != -1; }
  bool add(int fd, IOM_EVENTS events, PollHandler&& handler);
  constexpr bool modify(int fd, IOM_EVENTS events, std::optional<PollHandler>&& handler) {
    if (!this->valid()) {
      return false;
    }
    epoll_event event;
    event.events = (uint32_t)events;
    event.data.fd = fd;
    if (epoll_ctl(this->fd, EPOLL_CTL_MOD, fd, &event) == -1) {
      log_warning("Failed to modify handler for fd: {}, {}:{}", fd, errno, strerror(errno));
      return false;
    }
    if (handler.has_value()) {
      this->handlers.lock()->insert_or_assign(fd, make_shared<PollHandler>(std::move(*handler)));
    }
    return true;
  }
  constexpr void run() {
    while (this->valid()) {
      epoll_event events[10];
      sigset_t mask;
      sigemptyset(&mask);
      sigaddset(&mask, SIGINT);
      sigaddset(&mask, SIGTERM);
      sigaddset(&mask, SIGQUIT);
      int n = epoll_pwait(this->fd, events, 10, TIMEOUT, &mask);
      if (n == -1) {
        log_error("Failed to poll");
        continue;
      }
      // LOG6("Polling {} events", n);
      for (int i = 0; i < n; i++) {
        auto handler = this->handlers.lock_shared()->at(events[i].data.fd);
        auto fd = events[i].data.fd;
        auto ev = static_cast<IOM_EVENTS>(events[i].events);
        log_debug("Handling {} for fd: {}", to_string(ev), fd);
        PollHandleHint hint = (*this->proxy)(bind(std::ref(*handler), fd, ev));
        log_debug("HandleRes {} for fd: {}", to_string_view(hint.tag), (int)events[i].data.fd);
        switch (hint.tag) {
          case PollHandleHintTag::DELETE:
            this->remove(events[i].data.fd);
            break;
          case PollHandleHintTag::MODIFY:
            this->modify(events[i].data.fd, hint.data.events, std::nullopt);
            break;
          default:
            break;
        }
      }
    }
  }
  void remove(int fd);
  /// @brief shutdown the poller
  constexpr void shutdown() {
    if (!this->valid()) {
      return;
    }
    log_debug("call all handlers with NONE");
    for (auto& [key, value] : *this->handlers.lock()) {
      (*value)(key, IOM_EVENTS::NONE);
    }
    log_debug("close poller");
    close(this->fd);
    this->fd = -1;
  }

private:
  std::atomic_int fd;
  ShardRes<std::unordered_map<int, std::shared_ptr<PollHandler>>> handlers;
  std::shared_ptr<HandleProxy> proxy;
};

XSL_IO_NE
#endif
