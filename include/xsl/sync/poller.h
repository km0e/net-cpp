#pragma once
#ifndef _XSL_NET_POLLER_
#  define _XSL_NET_POLLER_
#  include "xsl/sync/sync.h"
#  include "xsl/wheel/hash_map.h"
#  include "xsl/wheel/wheel.h"

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
using PollHandler = wheel::function<void(int fd, IOM_EVENTS events)>;
class Poller {
public:
  virtual bool subscribe(int fd, IOM_EVENTS events, PollHandler&& handler) = 0;
  virtual bool modify(int fd, IOM_EVENTS events) = 0;
  virtual void poll() = 0;
  virtual void unregister(int fd) = 0;
  virtual void shutdown() = 0;
};
class EPoller;
using Handler = wheel::function<void(wheel::shared_ptr<EPoller> poller, int fd, IOM_EVENTS events)>;
class EPoller : public Poller {
public:
  EPoller();
  ~EPoller();
  bool valid();
  bool subscribe(int fd, IOM_EVENTS events, PollHandler&& handler) override;
  bool modify(int fd, IOM_EVENTS events) override;
  void poll() override;
  void unregister(int fd) override;
  void shutdown() override;

private:
  int fd;
  wheel::ConcurrentHashMap<int, wheel::shared_ptr<PollHandler>> handlers;
};
SYNC_NAMESPACE_END
#endif
