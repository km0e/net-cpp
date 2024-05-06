#include <spdlog/spdlog.h>
#include <sys/signal.h>
#include <xsl/config.h>
#include <xsl/sync/poller.h>
XSL_NAMESPACE_BEGIN
namespace sync {
  IOM_EVENTS operator|(IOM_EVENTS a, IOM_EVENTS b) {
    return (IOM_EVENTS)((uint32_t)a | (uint32_t)b);
  }
  IOM_EVENTS& operator|=(IOM_EVENTS& a, IOM_EVENTS b) {
    a = (IOM_EVENTS)((uint32_t)a | (uint32_t)b);
    return a;
  }
  IOM_EVENTS operator&(IOM_EVENTS a, IOM_EVENTS b) {
    return (IOM_EVENTS)((uint32_t)a & (uint32_t)b);
  }
  IOM_EVENTS& operator&=(IOM_EVENTS& a, IOM_EVENTS b) {
    a = (IOM_EVENTS)((uint32_t)a & (uint32_t)b);
    return a;
  }
  IOM_EVENTS operator~(IOM_EVENTS a) { return (IOM_EVENTS)(~(uint32_t)a); }
  EPoller::EPoller() {
    this->fd = epoll_create(1);
    spdlog::debug("[Poller::Poller] Poller fd: {}", this->fd);
  }
  bool EPoller::valid() { return this->fd != -1; }

  bool EPoller::register_handler(int fd, IOM_EVENTS events, PollHandler handler) {
    spdlog::trace("[Poller::register_handler]");
    epoll_event event;
    event.events = (uint32_t)events;
    event.data.fd = fd;
    if (epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &event) == -1) {
      return false;
    }
    spdlog::debug("[Poller::register_handler] Handler registered for fd: {}", fd);
    this->handlers[fd] = handler;
    return true;
  }
  bool EPoller::modify_handler(int fd, IOM_EVENTS events) {
    spdlog::trace("[Poller::modify_handler]");
    epoll_event event;
    event.events = (uint32_t)events;
    event.data.fd = fd;
    if (epoll_ctl(this->fd, EPOLL_CTL_MOD, fd, &event) == -1) {
      return false;
    }
    return true;
  }
  void EPoller::poll() {
    spdlog::trace("[Poller::poll]");
    epoll_event events[10];
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGQUIT);
    int n = epoll_pwait(this->fd, events, 10, -1, &mask);
    if (n == -1) {
      spdlog::error("[Poller::poll] Failed to poll");
      return;
    }
    spdlog::debug("[Poller::poll] Polling {} events", n);
    for (int i = 0; i < n; i++) {
      IOM_EVENTS event
          = this->handlers[events[i].data.fd](events[i].data.fd, (IOM_EVENTS)events[i].events);
      if (event == IOM_EVENTS::NONE) {
        this->unregister(events[i].data.fd);
      } else {
        this->modify_handler(events[i].data.fd, event);
      }
    }
  }
  void EPoller::unregister(int fd) { epoll_ctl(this->fd, EPOLL_CTL_DEL, fd, nullptr); }
  void EPoller::shutdown() { close(this->fd); }
  EPoller::~EPoller() { this->shutdown(); }
}  // namespace sync
XSL_NAMESPACE_END
