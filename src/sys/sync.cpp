/**
 * @file sync.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/sys/def.h"
#include "xsl/sys/sync.h"

#include <csignal>
#include <format>
XSL_SYS_NB
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
constexpr std::string to_string(IOM_EVENTS events) {
  // format to binary
  return std::format("{:b}", static_cast<uint32_t>(events));
}

Poller::Poller()
    : Poller(
          std::make_shared<HandleProxy>([](std::function<PollHandleHint()>&& f) { return f(); })) {}
Poller::Poller(std::shared_ptr<HandleProxy>&& proxy) : fd(-1), handlers(), proxy(std::move(proxy)) {
  this->fd = epoll_create(1);
  LOG5("Poller fd: {}", this->fd.load());
}
Poller::~Poller() { this->shutdown(); }

bool Poller::add(int fd, IOM_EVENTS events, PollHandler&& handler) {
  epoll_event event;
  event.events = static_cast<uint32_t>(events);
  event.data.fd = fd;
  auto guard = this->handlers.lock();
  // must be here, otherwise the handler may be not registered
  // in time when the event comes
  if (epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &event) == -1) {
    return false;
  }
  LOG5("Register {} for fd: {}", to_string(events), fd);
  guard->insert_or_assign(fd, make_shared<PollHandler>(std::move(handler)));
  return true;
}

void Poller::remove(int fd) {
  epoll_ctl(this->fd, EPOLL_CTL_DEL, fd, nullptr);
  (*this->handlers.lock()).erase(fd);
}

XSL_SYS_NE
