/**
 * @file sync.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Synchronization primitives
 * @version 0.12
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_SYNC
#  define XSL_SYS_SYNC
#  include "xsl/coro.h"
#  include "xsl/sync.h"
#  include "xsl/sys/def.h"

#  include <sys/epoll.h>

#  include <atomic>
#  include <concepts>
#  include <csignal>
#  include <cstdint>
#  include <functional>
#  include <memory>
#  include <string>
#  include <string_view>
#  include <utility>
XSL_SYS_NB
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
constexpr std::string to_string(IOM_EVENTS events);
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

namespace impl_poll {
  template <IOM_EVENTS... Events>
  class PollBase {
  public:
    constexpr IOM_EVENTS get_events() { return (Events | ...); }
  };

  template <IOM_EVENTS... Events>
  class PollCallback : public PollBase<Events...> {
  public:
    constexpr PollCallback(std::invocable<int, IOM_EVENTS> auto&&... callbacks)
        : _callbacks{std::forward<decltype(callbacks)>(callbacks)...} {}

    constexpr PollHandleHint operator()(int fd, IOM_EVENTS events) {
      return handle_event(fd, std::make_index_sequence<sizeof...(Events)>{}, events)
                 ? PollHandleHintTag::DELETE
                 : PollHandleHintTag::NONE;
    }

  private:
    std::array<std::function<PollHandleHint(int, IOM_EVENTS)>, sizeof...(Events)> _callbacks;

    template <std::size_t... I>
    constexpr bool handle_event(int fd, std::index_sequence<I...>, IOM_EVENTS events) {
      return (handle_event<Events, I>(fd, events) && ...);
    }

    template <IOM_EVENTS E, std::size_t I>
    constexpr bool handle_event(int fd, IOM_EVENTS events) {
      if (!!(events & E)) {
        auto hint = _callbacks[I](fd, E);
        if (hint.tag == PollHandleHintTag::DELETE) {
          return true;
        }
      }
      return false;
    }
  };
}  // namespace impl_poll
struct DefaultPollTraits {
  static constexpr PollHandleHintTag poll_check(IOM_EVENTS events) {
    return ((!events) || !!(events & IOM_EVENTS::HUP)) ? PollHandleHintTag::DELETE
                                                       : PollHandleHintTag::NONE;
  }
};

template <class Traits, class PubSub>
class PollForCoro {
public:
  constexpr PollForCoro(Traits, auto&& pubsub) : pubsub{std::forward<decltype(pubsub)>(pubsub)} {}

  constexpr PollHandleHint operator()(int, IOM_EVENTS events) {
    if (Traits::poll_check(events) == PollHandleHintTag::DELETE) {
      return PollHandleHintTag::DELETE;
    } else {
      pubsub.publish([&events](IOM_EVENTS e) { return !!(events & e); });
      return PollHandleHintTag::NONE;
    }
  }

private:
  PubSub pubsub;
};

template <class Traits, class PubSub>
PollForCoro(Traits, PubSub) -> PollForCoro<Traits, PubSub>;

template <IOM_EVENTS... Events>
using PollCallback = impl_poll::PollCallback<Events...>;

using HandleProxy = std::function<PollHandleHint(std::function<PollHandleHint()>&&)>;
class Poller {
public:
  Poller();
  Poller(std::shared_ptr<HandleProxy>&& proxy);
  ~Poller();
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
      WARN("Failed to modify handler for fd: {}, {}:{}", fd, errno, strerror(errno));
      return false;
    }
    if (handler.has_value()) {
      this->handlers.lock()->insert_or_assign(fd, make_shared<PollHandler>(std::move(*handler)));
    }
    return true;
  }
  constexpr void poll() {
    if (!this->valid()) {
      return;
    }
    // LOG6("Start polling");
    epoll_event events[10];
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGQUIT);
    int n = epoll_pwait(this->fd, events, 10, TIMEOUT, &mask);
    if (n == -1) {
      LOG2("Failed to poll");
      return;
    }
    // LOG6("Polling {} events", n);
    for (int i = 0; i < n; i++) {
      auto handler = this->handlers.lock_shared()->at(events[i].data.fd);
      auto fd = events[i].data.fd;
      auto ev = static_cast<IOM_EVENTS>(events[i].events);
      LOG6("Handling {} for fd: {}", static_cast<uint32_t>(ev), fd);
      PollHandleHint hint = (*this->proxy)(bind(std::ref(*handler), fd, ev));
      LOG5("HandleRes {} for fd: {}", to_string_view(hint.tag), (int)events[i].data.fd);
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
    // LOG6("Polling done");
  }
  void remove(int fd);
  /// @brief shutdown the poller
  constexpr void shutdown() {
    if (!this->valid()) {
      return;
    }
    LOG5("call all handlers with NONE");
    for (auto& [key, value] : *this->handlers.lock()) {
      (*value)(key, IOM_EVENTS::NONE);
    }
    LOG5("close poller");
    close(this->fd);
    this->fd = -1;
  }

private:
  std::atomic_int fd;
  ShardRes<std::unordered_map<int, std::shared_ptr<PollHandler>>> handlers;
  std::shared_ptr<HandleProxy> proxy;
};

/**
 * @brief Poll by signal
 *
 * @tparam Traits the poll traits
 * @param poller the poller
 * @param fd the file descriptor
 * @param events the events
 * @return auto the signal
 */
template <class Traits>
constexpr auto poll_by_signal(Poller& poller, int fd, std::same_as<IOM_EVENTS> auto... events) {
  auto add = [&]<class _PubSub>(_PubSub&& pubsub, auto&&... rxs) {
    poller.add(fd, (events | ...) | IOM_EVENTS::ET, PollForCoro{Traits{}, std::move(pubsub)});
    return std::make_tuple(std::forward<decltype(rxs)>(rxs)...);
  };

  return [&add]<std::size_t... I>(std::index_sequence<I...>, auto&& pubsub) {
    return add(std::get<I>(std::forward<decltype(pubsub)>(pubsub))...);
  }(std::make_index_sequence<sizeof...(events) + 1>{},
         make_exact_pub_sub<IOM_EVENTS, SPSCSignal<1>>(events...));
}
XSL_SYS_NE
#endif
