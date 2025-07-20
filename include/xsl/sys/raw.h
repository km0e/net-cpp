/**
 * @file raw.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Raw device
 * @version 0.2.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_RAW
#  define XSL_SYS_RAW
#  include "xsl/sys/def.h"

#  include <fcntl.h>
#  include <sys/socket.h>
#  include <unistd.h>

#  include <expected>
#  include <system_error>
#  include <utility>
XSL_SYS_NB
using RawHandle = int;  ///< RawHandle is a file descriptor type

/// @brief RawOwner is a wrapper for file descriptor
struct RawOwner {
  int fd;

  constexpr RawOwner() noexcept : fd(-1) {}
  explicit constexpr RawOwner(int fd) noexcept : fd(fd) {}
  constexpr RawOwner(RawOwner &&rhs) noexcept : fd(std::exchange(rhs.fd, -1)) {}
  constexpr RawOwner &operator=(RawOwner &&rhs) noexcept {
    fd = std::exchange(rhs.fd, -1);
    return *this;
  }
  constexpr ~RawOwner() noexcept {
    if (fd >= 0) {
      ::close(fd);
    }
  }
  /// @brief get raw file descriptor
  constexpr auto &&raw(this auto &&self) noexcept { return std::forward<decltype(self)>(self).fd; }
  /// @brief check if the file descriptor is valid
  constexpr bool is_valid() const noexcept { return fd >= 0; }
};

/**
 * @brief Set the blocking object
 *
 * @tparam is_blocking true if the object is blocking, false otherwise
 * @param fd the file descriptor
 * @return std::expected<void, errc>
 */
template <bool is_blocking>
constexpr std::expected<void, errc> set_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return std::unexpected{errc(errno)};
  }
  if constexpr (!is_blocking) {
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
      return std::unexpected{errc(errno)};
    }
  } else {
    if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
      return std::unexpected{errc(errno)};
    }
  }
  return {};
}
/**
 * @brief Set the blocking object
 *
 * @param fd the file descriptor
 * @param blocking true if the object is blocking, false otherwise
 * @return std::expected<void, errc>
 */
std::expected<void, errc> set_blocking(int fd, bool blocking);
/**
 * @brief Filter the interrupt
 *
 * @tparam F the function type
 * @tparam Args the arguments type
 * @param f the function
 * @param args the arguments
 * @return int the return value of the function
 */
template <class F>
constexpr int filter_interrupt(F &&f, auto &&...args) {
  int ret;
  do {
    ret = f(std::forward<decltype(args)>(args)...);
  } while (ret == -1 && errno == EINTR);
  return ret;
}

constexpr errc check_ec(int ret) {
  if (ret == -1) {
    return errc{errno};
  }
  return {};
}

inline std::error_condition current_ec() { return std::make_error_condition(errc{errno}); }

// TODO: Implement the timer
//  int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
//  if (timer_fd == -1) {
//    LOG2("Failed to create timerfd, error: {}", strerror(errno));
//    return;
//  }
//  this->poller->add(timer_fd, IOM_EVENTS::IN,
//                    [this](int fd, IOM_EVENTS events) { return (*this)(fd, events); });
//  struct itimerspec new_value;
//  new_value.it_value.tv_sec = limit.recv_timeout / 1000;
//  new_value.it_value.tv_nsec = (limit.recv_timeout % 1000) * 1000000;
//  new_value.it_interval.tv_sec = limit.recv_timeout / 1000;
//  new_value.it_interval.tv_nsec = (limit.recv_timeout % 1000) * 1000000;
//  if (timerfd_settime(timer_fd, 0, &new_value, nullptr) == -1) {
//    LOG2("Failed to set timerfd, error: {}", strerror(errno));
//  }
XSL_SYS_NE
#endif
