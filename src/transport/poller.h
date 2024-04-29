#pragma once
#ifndef _XSL_NET_TRANSPORT_POLLER_H_
#define _XSL_NET_TRANSPORT_POLLER_H_
#include <transport.h>
#include <wheel.h>

namespace xsl {
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
}  // namespace xsl
#endif