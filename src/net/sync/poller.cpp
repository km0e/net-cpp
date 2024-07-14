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
  DEBUG("Poller fd: {}", this->fd);
}
bool Poller::valid() { return this->fd != -1; }

bool Poller::add(int fd, IOM_EVENTS events, PollHandler&& handler) {
  epoll_event event;
  event.events = (uint32_t)events;
  event.data.fd = fd;
  if (epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &event) == -1) {
    return false;
  }
  DEBUG("Handler registered for fd: {}", fd);
  // there should be a lock here?
  this->handlers.lock()->try_emplace(fd, make_shared<PollHandler>(handler));
  return true;
}
bool Poller::modify(int fd, IOM_EVENTS events, std::optional<PollHandler>&& handler) {
  epoll_event event;
  event.events = (uint32_t)events;
  event.data.fd = fd;
  if (epoll_ctl(this->fd, EPOLL_CTL_MOD, fd, &event) == -1) {
    return false;
  }
  if (handler.has_value()) {
    this->handlers.lock()->insert_or_assign(fd, make_shared<PollHandler>(handler.value()));
  }
  return true;
}
void Poller::poll() {
  if (!this->valid()) {
    return;
  }
  // TRACE("Start polling");
  epoll_event events[10];
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGQUIT);
  int n = epoll_pwait(this->fd, events, 10, TIMEOUT, &mask);
  if (n == -1) {
    ERROR("Failed to poll");
    return;
  }
  // TRACE("Polling {} events", n);
  for (int i = 0; i < n; i++) {
    auto handler = this->handlers.lock_shared()->at(events[i].data.fd);
    PollHandleHint hint
        = (*this->proxy)(bind(*handler, (int)events[i].data.fd, (IOM_EVENTS)events[i].events));
    DEBUG("Handling {} for fd: {}", to_string(hint.tag), (int)events[i].data.fd);
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
  // TRACE("Polling done");
}
void Poller::remove(int fd) {
  epoll_ctl(this->fd, EPOLL_CTL_DEL, fd, nullptr);
  (*this->handlers.lock()).erase(fd);
}
void Poller::shutdown() {
  if (!this->valid()) {
    return;
  }
  DEBUG("call all handlers with NONE");
  for (auto& [key, value] : *this->handlers.lock()) {
    (*value)(key, IOM_EVENTS::NONE);
  }
  this->handlers.lock()->clear();
  DEBUG("close poller");
  close(this->fd);
  this->fd = -1;
}
Poller::~Poller() { this->shutdown(); }
XSL_SYNC_NE
