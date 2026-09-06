/**
 * @file context.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Context for I/O operations
 * @version 0.1.2
 * @date 2025-06-02
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_SYS_IO_CONTEXT_EPOLL
#  define XSL_SYS_IO_CONTEXT_EPOLL
#  include <sys/epoll.h>
#  include <xsl/compose.h>
#  include <xsl/error.h>
#  include <xsl/io/def.h>
#  include <xsl/sync.h>
#  include <xsl/sys/def.h>

#  include <csignal>
#  include <memory>
XSL_SYS_NB

const int TIMEOUT = 100;

#  define USE_EPOLL
#  ifdef USE_EPOLL

class IOContext;

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

class EpollHandler {
public:
  virtual ~EpollHandler() = default;

  friend class IOContext;

  virtual PollHandleHint epoll_handle(int fd, IOM_EVENTS events) = 0;
  /// @brief wake all subscribers of this handler (used on shutdown)
  virtual void shutdown_notify() {}
};

class IOContext {
  IOContext(int fd);

public:
  static Expected<IOContext*, errc> create() {
    TRVEC(fd, epoll_create1(EPOLL_CLOEXEC));
    log_debug("Poller fd: {}", fd);
    return {new IOContext(fd)};
  }

  ~IOContext();
  constexpr bool valid() noexcept { return this->fd != -1; }
  /// @brief whether shutdown has been initiated
  constexpr bool stopped() const noexcept {
    return this->stopped_.load(std::memory_order_acquire);
  }
  template <class T>
    requires std::is_same_v<std::remove_cvref_t<T>, shared_memory<EpollHandler>>
             || std::is_constructible_v<T, T&&>
  Expected<void, errc> add(int fd, IOM_EVENTS events, T&& handler) {
    epoll_event event;
    event.events = static_cast<uint32_t>(events);
    event.data.fd = fd;
    auto guard = this->handlers.lock();
    // must be here, otherwise the handler may be not registered
    // in time when the event comes
    ENSEC(epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &event) == 0);
    log_debug("Register {} for fd: {}", to_string(events), fd);
    guard->insert_or_assign(fd, std::forward<T>(handler));
    return {};
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
      for (int i = 0; i < n; i++) {
        // pin the handler: it may be deregistered concurrently (e.g. the
        // connection closed while this event batch was in flight)
        std::optional<shared_memory<EpollHandler>> handler;
        {
          auto guard = this->handlers.lock_shared();
          if (auto iter = guard->find(events[i].data.fd); iter != guard->end()) {
            handler.emplace(iter->second);
          }
        }
        if (!handler) {
          continue;
        }
        auto fd = events[i].data.fd;
        auto ev = static_cast<IOM_EVENTS>(events[i].events);
        log_debug("Handling {} for fd: {}", to_string(ev), fd);
        PollHandleHint hint = (*handler)->epoll_handle(fd, ev);
        log_debug("HandleRes {} for fd: {}", to_string_view(hint.tag), (int)events[i].data.fd);
        switch (hint.tag) {
          case PollHandleHintTag::DELETE:
            this->remove(events[i].data.fd);
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
    // mark stopped BEFORE waking tasks: any task that resumes from this point
    // on must observe the shutdown and stop re-arming its suspension
    this->stopped_.store(true, std::memory_order_release);
    // wake every subscriber, otherwise tasks suspended on their signals would
    // never resume (a NONE event matches no IOM_EVENTS key)
    for (auto& [key, value] : *this->handlers.lock_shared()) {
      value->shutdown_notify();
    }
    // drop all handlers so their fds are closed; tasks still owning their own
    // references keep the objects alive until they finish
    this->handlers.lock()->clear();
    log_debug("close poller");
    close(this->fd);
    this->fd = -1;
  }

private:
  std::atomic_int fd;
  std::atomic_bool stopped_{false};
  ShardRes<std::unordered_map<int, shared_memory<EpollHandler>>> handlers;
};

XSL_SYS_NE
#endif
