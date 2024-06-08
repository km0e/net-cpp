#pragma once
#ifndef _XSL_NET_POLLER_
#  define _XSL_NET_POLLER_
#  include "xsl/net/sync/def.h"
#  include "xsl/wheel.h"

#  include <sys/epoll.h>
#  include <sys/socket.h>
#  include <sys/types.h>

#  include <functional>
SYNC_NAMESPACE_BEGIN
#  define USE_EPOLL
#  ifdef USE_EPOLL
enum class IOM_EVENTS : uint32_t {
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

class Poller {
public:
  virtual bool add(int fd, IOM_EVENTS events, PollHandler&& handler) = 0;
  virtual bool modify(int fd, IOM_EVENTS events, std::optional<PollHandler>&& handler) = 0;
  virtual void poll() = 0;  // NOLINT [runtime/references
  virtual void remove(int fd) = 0;
  virtual void shutdown() = 0;
  virtual ~Poller() = default;
};
using HandleProxy = std::function<PollHandleHint(std::function<PollHandleHint()>&&)>;
class DefaultPoller : public Poller {
public:
  DefaultPoller();
  DefaultPoller(std::shared_ptr<HandleProxy>&& proxy);
  ~DefaultPoller();
  bool valid();
  bool add(int fd, IOM_EVENTS events, PollHandler&& handler) override;
  bool modify(int fd, IOM_EVENTS events, std::optional<PollHandler>&& handler) override;
  void poll() override;
  void remove(int fd) override;
  void shutdown() override;

private:
  int fd;
  ShrdRes<std::unordered_map<int, std::shared_ptr<PollHandler>>> handlers;
  std::shared_ptr<HandleProxy> proxy;
};
template <Handler T, class... Args>
std::unique_ptr<T> poll_add_unique(std::shared_ptr<Poller> poller, int fd, IOM_EVENTS events,
                                   Args&&... args) {
  auto handler = std::make_unique<T>(std::forward<Args>(args)...);
  poller->add(fd, events, [handler = handler.get()](int fd, IOM_EVENTS events) {
    return (*handler)(fd, events);
  });
  return handler;
}
template <Handler T, class... Args>
std::shared_ptr<T> poll_add_shared(std::shared_ptr<Poller> poller, int fd, IOM_EVENTS events,
                                   Args&&... args) {
  auto handler = std::make_shared<T>(std::forward<Args>(args)...);
  poller->add(fd, events, [handler = handler.get()](int fd, IOM_EVENTS events) {
    return (*handler)(fd, events);
  });
  return handler;
}
SYNC_NAMESPACE_END
#endif
