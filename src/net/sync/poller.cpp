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

DefaultPoller::DefaultPoller()
    : DefaultPoller(
        std::make_shared<HandleProxy>([](std::function<PollHandleHint()>&& f) { return f(); })) {}
DefaultPoller::DefaultPoller(std::shared_ptr<HandleProxy>&& proxy)
    : fd(-1), handlers(), proxy(std::move(proxy)) {
  this->fd = epoll_create(1);
  SPDLOG_DEBUG("Poller fd: {}", this->fd);
}
bool DefaultPoller::valid() { return this->fd != -1; }

bool DefaultPoller::add(int fd, IOM_EVENTS events, PollHandler&& handler) {
  epoll_event event;
  event.events = (uint32_t)events;
  event.data.fd = fd;
  if (epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &event) == -1) {
    return false;
  }
  SPDLOG_DEBUG("Handler registered for fd: {}", fd);
  // there should be a lock here?
  this->handlers.lock()->try_emplace(fd, make_shared<PollHandler>(handler));
  return true;
}
bool DefaultPoller::modify(int fd, IOM_EVENTS events, std::optional<PollHandler>&& handler) {
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
void DefaultPoller::poll() {
  if (!this->valid()) {
    return;
  }
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
    auto handler = this->handlers.lock_shared()->at(events[i].data.fd);
    PollHandleHint hint
        = (*this->proxy)(bind(*handler, (int)events[i].data.fd, (IOM_EVENTS)events[i].events));
    SPDLOG_DEBUG("Handling {} for fd: {}", to_string(hint.tag), (int)events[i].data.fd);
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
  SPDLOG_TRACE("Polling done");
}
void DefaultPoller::remove(int fd) {
  epoll_ctl(this->fd, EPOLL_CTL_DEL, fd, nullptr);
  (*this->handlers.lock()).erase(fd);
}
void DefaultPoller::shutdown() {
  if (!this->valid()) {
    return;
  }
  for (auto& [key, value] : *this->handlers.lock()) {
    close(key);
  }
  this->handlers.lock()->clear();
  close(this->fd);
  this->fd = -1;
}
DefaultPoller::~DefaultPoller() { this->shutdown(); }
SYNC_NAMESPACE_END
