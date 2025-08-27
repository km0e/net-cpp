/**
 * @file context.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <xsl/error.h>
#include <xsl/io/context.h>

XSL_IO_NB
constexpr IOM_EVENTS& operator|=(IOM_EVENTS& a, IOM_EVENTS b) {
  a = static_cast<IOM_EVENTS>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
  return a;
}
constexpr IOM_EVENTS& operator&=(IOM_EVENTS& a, IOM_EVENTS b) {
  a = static_cast<IOM_EVENTS>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
  return a;
}
constexpr IOM_EVENTS operator~(IOM_EVENTS a) {
  return static_cast<IOM_EVENTS>(~static_cast<uint32_t>(a));
}

Context::Context()
    : Context(
          std::make_shared<HandleProxy>([](std::function<PollHandleHint()>&& f) { return f(); })) {}
Context::Context(std::shared_ptr<HandleProxy>&& proxy)
    : fd(-1), handlers(), proxy(std::move(proxy)) {
  this->fd = epoll_create(1);
  log_debug("Poller fd: {}", this->fd.load());
}
Context::~Context() { this->shutdown(); }

Expected<void, errc> Context::add(int fd, IOM_EVENTS events, PollHandler&& handler) {
  epoll_event event;
  event.events = static_cast<uint32_t>(events);
  event.data.fd = fd;
  auto guard = this->handlers.lock();
  // must be here, otherwise the handler may be not registered
  // in time when the event comes
  ENSEC(epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &event) == 0);
  log_debug("Register {} for fd: {}", to_string(events), fd);
  guard->insert_or_assign(fd, make_shared<PollHandler>(std::move(handler)));
  return {};
}

void Context::remove(int fd) {
  epoll_ctl(this->fd, EPOLL_CTL_DEL, fd, nullptr);
  (*this->handlers.lock()).erase(fd);
}

XSL_IO_NE
