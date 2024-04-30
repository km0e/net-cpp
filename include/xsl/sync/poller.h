#pragma once
#ifndef _XSL_NET_POLLER_
#define _XSL_NET_POLLER_
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <xsl/config.h>
#include <xsl/utils/wheel/wheel.h>
XSL_NAMESPACE_BEGIN
namespace sync {
#define USE_EPOLL
#ifdef USE_EPOLL
  enum IOM_EVENTS {
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
#endif
  class Poller;
  using Handler = wheel::function<void(wheel::shared_ptr<Poller> poller, int fd, IOM_EVENTS events)>;
  using PollHandler = wheel::function<bool(int fd, IOM_EVENTS events)>;
  class Poller {
  public:
    Poller();
    ~Poller();
    bool valid();
    bool register_handler(int fd, IOM_EVENTS events, PollHandler handler);
    void poll();
    void unregister(int fd);
    void shutdown();
  private:
    int fd;
    wheel::unordered_map<int, PollHandler> handlers;
  };
}  // namespace sync
XSL_NAMESPACE_END
#endif