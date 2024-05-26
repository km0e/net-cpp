#pragma once
#ifndef _XSL_NET_POLLER_
#  define _XSL_NET_POLLER_
#  include "xsl/net/sync/def.h"
#  include "xsl/wheel.h"

#  include <sys/epoll.h>
#  include <sys/socket.h>
#  include <sys/types.h>
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
template <class T>
concept Handler = requires(T t) {
  { t(0, IOM_EVENTS::NONE) };
};
using PollHandler = function<void(int fd, IOM_EVENTS events)>;
class Poller {
public:
  virtual bool subscribe(int fd, IOM_EVENTS events, PollHandler&& handler) = 0;
  virtual bool modify(int fd, IOM_EVENTS events) = 0;
  virtual void poll() = 0;  // NOLINT [runtime/references
  virtual void unregister(int fd) = 0;
  virtual void shutdown() = 0;
  virtual ~Poller() = default;
};
using HandleProxy = function<void(function<void()>&&)>;
class DefaultPoller : public Poller {
public:
  DefaultPoller();
  DefaultPoller(shared_ptr<HandleProxy>&& proxy);
  ~DefaultPoller();
  bool valid();
  bool subscribe(int fd, IOM_EVENTS events, PollHandler&& handler) override;
  bool modify(int fd, IOM_EVENTS events) override;
  void poll() override;
  void unregister(int fd) override;
  void shutdown() override;

private:
  int fd;
  ShareContainer<unordered_map<int, shared_ptr<PollHandler>>> handlers;
  shared_ptr<HandleProxy> proxy;
};
template <Handler T, class... Args>
unique_ptr<T> sub_unique(shared_ptr<Poller> poller, int fd, IOM_EVENTS events, Args&&... args) {
  auto handler = make_unique<T>(forward<Args>(args)...);
  poller->subscribe(
      fd, events, [handler = handler.get()](int fd, IOM_EVENTS events) { (*handler)(fd, events); });
  return handler;
}
template <Handler T, class... Args>
shared_ptr<T> sub_shared(shared_ptr<Poller> poller, int fd, IOM_EVENTS events, Args&&... args) {
  auto handler = make_shared<T>(forward<Args>(args)...);
  poller->subscribe(
      fd, events, [handler = handler.get()](int fd, IOM_EVENTS events) { (*handler)(fd, events); });
  return handler;
}
SYNC_NAMESPACE_END
#endif
