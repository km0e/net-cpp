#pragma once
#ifndef XSL_SYS_RAW
#  define XSL_SYS_RAW
#  include "xsl/logctl.h"
#  include "xsl/sys/def.h"

#  include <fcntl.h>
#  include <unistd.h>

#  include <expected>
#  include <system_error>
#  include <utility>
XSL_SYS_NB

template <class T>
concept RawDeviceLike = requires(T t) {
  { t.raw() };
};

class RawDevice {
public:
  RawDevice(int fd) noexcept : _fd(fd) {}
  RawDevice(RawDevice &&rhs) noexcept : _fd(std::exchange(rhs._fd, -1)) {}
  RawDevice &operator=(RawDevice &&rhs) noexcept {
    _fd = std::exchange(rhs._fd, -1);
    return *this;
  }
  int raw() const noexcept { return _fd; }

  ~RawDevice() noexcept {
    if (_fd == -1) {
      return;
    }
    LOG6("close fd: {}", _fd);
    close(_fd);
  }

protected:
  int _fd;
};

template <bool is_blocking>
std::expected<void, std::errc> set_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return std::unexpected{std::errc(errno)};
  }
  if constexpr (!is_blocking) {
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
      return std::unexpected{std::errc(errno)};
    }
  } else {
    if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
      return std::unexpected{std::errc(errno)};
    }
  }
  return {};
}
std::expected<void, std::errc> set_blocking(int fd, bool blocking);

XSL_SYS_NE
#endif
