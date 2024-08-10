#include "xsl/logctl.h"
#include "xsl/sync/def.h"
#include "xsl/sync/poller.h"

#include <sys/signal.h>

#include <cstdint>
XSL_SYNC_NB
IOM_EVENTS operator|(IOM_EVENTS a, IOM_EVENTS b) {
  return static_cast<IOM_EVENTS>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
IOM_EVENTS& operator|=(IOM_EVENTS& a, IOM_EVENTS b) {
  a = static_cast<IOM_EVENTS>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
  return a;
}
IOM_EVENTS operator&(IOM_EVENTS a, IOM_EVENTS b) {
  return static_cast<IOM_EVENTS>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
IOM_EVENTS& operator&=(IOM_EVENTS& a, IOM_EVENTS b) {
  a = static_cast<IOM_EVENTS>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
  return a;
}
IOM_EVENTS operator~(IOM_EVENTS a) { return static_cast<IOM_EVENTS>(~static_cast<uint32_t>(a)); }
bool operator!(IOM_EVENTS a) { return a == IOM_EVENTS::NONE; }

std::string_view to_string(PollHandleHintTag tag) {
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

Poller::Poller()
    : Poller(
          std::make_shared<HandleProxy>([](std::function<PollHandleHint()>&& f) { return f(); })) {}
Poller::Poller(std::shared_ptr<HandleProxy>&& proxy) : fd(-1), handlers(), proxy(std::move(proxy)) {
  this->fd = epoll_create(1);
  LOG5("Poller fd: {}", this->fd.load());
}
bool Poller::valid() { return this->fd != -1; }

bool Poller::add(int fd, IOM_EVENTS events, PollHandler&& handler) {
  epoll_event event;
  event.events = static_cast<uint32_t>(events);
  event.data.fd = fd;
  if (epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &event) == -1) {
    return false;
  }
  LOG5("Register {} for fd: {}", static_cast<uint32_t>(events), fd);
  // there should be a lock here?
  this->handlers.lock()->insert_or_assign(fd, make_shared<PollHandler>(handler));
  return true;
}
bool Poller::modify(int fd, IOM_EVENTS events, std::optional<PollHandler>&& handler) {
  epoll_event event;
  event.events = (uint32_t)events;
  event.data.fd = fd;
  if (epoll_ctl(this->fd, EPOLL_CTL_MOD, fd, &event) == -1) {
    WARN("Failed to modify handler for fd: {}, {}:{}", fd, errno, strerror(errno));
    return false;
  }
  if (handler.has_value()) {
    this->handlers.lock()->insert_or_assign(fd, make_shared<PollHandler>(*handler));
  }
  return true;
}
void Poller::poll() {
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
    PollHandleHint hint = (*this->proxy)(bind(*handler, fd, ev));
    LOG5("HandleRes {} for fd: {}", to_string(hint.tag), (int)events[i].data.fd);
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
void Poller::remove(int fd) {
  epoll_ctl(this->fd, EPOLL_CTL_DEL, fd, nullptr);
  (*this->handlers.lock()).erase(fd);
}
void Poller::shutdown() {
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
Poller::~Poller() { this->shutdown(); }
XSL_SYNC_NE
