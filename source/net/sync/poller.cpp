#include "xsl/net/sync/def.h"
#include "xsl/net/sync/poller.h"

#include <spdlog/spdlog.h>
#include <sys/signal.h>
SYNC_NAMESPACE_BEGIN
IOM_EVENTS operator|(IOM_EVENTS a, IOM_EVENTS b) { return (IOM_EVENTS)((uint32_t)a | (uint32_t)b); }
IOM_EVENTS& operator|=(IOM_EVENTS& a, IOM_EVENTS b) {
  a = (IOM_EVENTS)((uint32_t)a | (uint32_t)b);
  return a;
}
IOM_EVENTS operator&(IOM_EVENTS a, IOM_EVENTS b) { return (IOM_EVENTS)((uint32_t)a & (uint32_t)b); }
IOM_EVENTS& operator&=(IOM_EVENTS& a, IOM_EVENTS b) {
  a = (IOM_EVENTS)((uint32_t)a & (uint32_t)b);
  return a;
}
IOM_EVENTS operator~(IOM_EVENTS a) { return (IOM_EVENTS)(~(uint32_t)a); }
DefaultPoller::DefaultPoller()
    : DefaultPoller(wheel::make_shared<HandleProxy>([](wheel::function<void()>&& f) { f(); })) {}
DefaultPoller::DefaultPoller(wheel::shared_ptr<HandleProxy>&& proxy)
    : fd(-1), handlers(), proxy(wheel::move(proxy)) {
  this->fd = epoll_create(1);
  SPDLOG_DEBUG("Poller fd: {}", this->fd);
}
bool DefaultPoller::valid() { return this->fd != -1; }

bool DefaultPoller::subscribe(int fd, IOM_EVENTS events, PollHandler&& handler) {
  SPDLOG_TRACE("");
  epoll_event event;
  event.events = (uint32_t)events;
  event.data.fd = fd;
  if (epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &event) == -1) {
    return false;
  }
  SPDLOG_DEBUG("Handler registered for fd: {}", fd);
  // there should be a lock here?
  this->handlers.lock()->try_emplace(fd, wheel::make_shared<PollHandler>(handler));
  return true;
}
bool DefaultPoller::modify(int fd, IOM_EVENTS events) {
  SPDLOG_TRACE("");
  epoll_event event;
  event.events = (uint32_t)events;
  event.data.fd = fd;
  if (epoll_ctl(this->fd, EPOLL_CTL_MOD, fd, &event) == -1) {
    return false;
  }
  return true;
}
void DefaultPoller::poll() {
  SPDLOG_TRACE("Start polling");
  epoll_event events[10];
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGQUIT);
  int n = epoll_pwait(this->fd, events, 10, -1, &mask);
  if (n == -1) {
    SPDLOG_ERROR("Failed to poll");
    return;
  }
  SPDLOG_DEBUG("Polling {} events", n);
  for (int i = 0; i < n; i++) {
    auto handler = this->handlers.share()->at(events[i].data.fd);
    (*this->proxy)(wheel::bind(*handler, (int)events[i].data.fd, (IOM_EVENTS)events[i].events));
  }
  SPDLOG_TRACE("Polling done");
}
void DefaultPoller::unregister(int fd) {
  epoll_ctl(this->fd, EPOLL_CTL_DEL, fd, nullptr);
  (*this->handlers.lock()).erase(fd);
}
void DefaultPoller::shutdown() { close(this->fd); }
DefaultPoller::~DefaultPoller() { this->shutdown(); }
SYNC_NAMESPACE_END
