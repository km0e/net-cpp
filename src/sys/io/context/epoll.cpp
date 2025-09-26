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
#include <xsl/sys/io/context/epoll.h>

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
IOContext::IOContext(int fd) : fd(fd), handlers() {}

IOContext::~IOContext() { this->shutdown(); }

void IOContext::remove(int fd) {
  epoll_ctl(this->fd, EPOLL_CTL_DEL, fd, nullptr);
  (*this->handlers.lock()).erase(fd);
}

XSL_SYS_NE
