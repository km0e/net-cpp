/**
 * @file sync.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Synchronization primitives
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_SYNC
#  define XSL_SYS_SYNC
#  include "xsl/coro.h"
#  include "xsl/feature.h"
#  include "xsl/sync.h"
#  include "xsl/sys/def.h"

#  include <sys/epoll.h>

#  include <atomic>
#  include <concepts>
#  include <cstdint>
#  include <functional>
#  include <memory>
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
IOM_EVENTS operator|(IOM_EVENTS a, IOM_EVENTS b);
IOM_EVENTS& operator|=(IOM_EVENTS& a, IOM_EVENTS b);
IOM_EVENTS operator&(IOM_EVENTS a, IOM_EVENTS b);
IOM_EVENTS& operator&=(IOM_EVENTS& a, IOM_EVENTS b);
IOM_EVENTS operator~(IOM_EVENTS a);
bool operator!(IOM_EVENTS a);
#  endif

enum class PollHandleHintTag : uint8_t {
  NONE = 0,
  MODIFY = 1,
  DELETE = 2,
};

std::string_view to_string(PollHandleHintTag tag);

class PollHandleHint {
public:
  PollHandleHintTag tag;
  union {
    IOM_EVENTS events;
  } data;
  PollHandleHint() : tag(PollHandleHintTag::NONE), data{IOM_EVENTS::NONE} {}
  PollHandleHint(PollHandleHintTag tag) : tag(tag), data{IOM_EVENTS::NONE} {}
  PollHandleHint(PollHandleHintTag tag, IOM_EVENTS events) : tag(tag), data{events} {}
};
template <class T>
concept Handler = requires(T t) {
  { t(0, IOM_EVENTS::NONE) } -> std::same_as<PollHandleHint>;
};

using PollHandler = std::function<PollHandleHint(int fd, IOM_EVENTS events)>;

namespace impl_poll {
  template <IOM_EVENTS... Events>
  class PollBase {
  public:
    IOM_EVENTS get_events() { return (Events | ...); }
  };

  template <class Traits, IOM_EVENTS... Events>
  class PollForCoro : public PollBase<Events...> {
  public:
    PollForCoro(std::array<std::shared_ptr<BinarySignal>, sizeof...(Events)> sems)
        : sems(std::move(sems)) {}

    template <class... Sems>
    PollForCoro(Sems&&... sems) : sems{std::forward<Sems>(sems)...} {}
    PollHandleHint operator()(int, IOM_EVENTS events) {
      if (Traits::poll_check(events) == PollHandleHintTag::DELETE) {
        for (auto& sem : sems) {
          sem->release(false);
        }
        return PollHandleHintTag::DELETE;
      } else {
        return handle_event(std::make_index_sequence<sizeof...(Events)>{}, events)
                   ? PollHandleHintTag::DELETE
                   : PollHandleHintTag::NONE;
      }
    }

  private:
    std::array<std::shared_ptr<BinarySignal>, sizeof...(Events)> sems;

    template <std::size_t... I>
    bool handle_event(std::index_sequence<I...>, IOM_EVENTS events) {
      return (handle_event<Events, I>(events) && ...);
    }

    template <IOM_EVENTS E, std::size_t I>
    bool handle_event(IOM_EVENTS events) {
      if (sems[I].use_count() > 1) {
        if (!!(events & E)) {
          sems[I]->release(true);
        }
        return false;
      }
      return true;
    }
  };
  template <IOM_EVENTS... Events>
  class PollCallback : public PollBase<Events...> {
  public:
    template <std::invocable<int, IOM_EVENTS>... Callbacks>
    PollCallback(Callbacks&&... callbacks) : _callbacks{std::forward<Callbacks>(callbacks)...} {}

    PollHandleHint operator()(int fd, IOM_EVENTS events) {
      return handle_event(fd, std::make_index_sequence<sizeof...(Events)>{}, events)
                 ? PollHandleHintTag::DELETE
                 : PollHandleHintTag::NONE;
    }

  private:
    std::array<std::function<PollHandleHint(int, IOM_EVENTS)>, sizeof...(Events)> _callbacks;

    template <std::size_t... I>
    bool handle_event(int fd, std::index_sequence<I...>, IOM_EVENTS events) {
      return (handle_event<Events, I>(fd, events) && ...);
    }

    template <IOM_EVENTS E, std::size_t I>
    bool handle_event(int fd, IOM_EVENTS events) {
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
  static PollHandleHintTag poll_check(IOM_EVENTS events) {
    return ((!events) || !!(events & IOM_EVENTS::HUP)) ? PollHandleHintTag::DELETE
                                                       : PollHandleHintTag::NONE;
  }
};

template <class Traits, IOM_EVENTS... Events>
using PollForCoro = impl_poll::PollForCoro<Traits, Events...>;

template <IOM_EVENTS... Events>
using PollCallback = impl_poll::PollCallback<Events...>;

using HandleProxy = std::function<PollHandleHint(std::function<PollHandleHint()>&&)>;
class Poller {
public:
  Poller();
  Poller(std::shared_ptr<HandleProxy>&& proxy);
  ~Poller();
  bool valid();
  bool add(int fd, IOM_EVENTS events, PollHandler&& handler);
  template <class Poll>
  bool add(int fd, Poll&& poll) {
    return add(fd, poll.get_events() | IOM_EVENTS::ET, std::forward<Poll>(poll));
  }
  bool modify(int fd, IOM_EVENTS events, std::optional<PollHandler>&& handler);
  template <class Poll>
  bool modify(int fd, Poll&& poll) {
    return modify(fd, poll.get_events() | IOM_EVENTS::ET, std::forward<Poll>(poll));
  }
  void poll();
  void remove(int fd);
  /**
   * @brief Shutdown the poller
   *
   */
  void shutdown();

private:
  std::atomic_int fd;
  ShardRes<std::unordered_map<int, std::shared_ptr<PollHandler>>> handlers;
  std::shared_ptr<HandleProxy> proxy;
};

XSL_SYS_NE
#endif
