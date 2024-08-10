#pragma once
#ifndef XSL_SYS_RAW
#  define XSL_SYS_RAW
#  include "xsl/sys/def.h"

#  include <fcntl.h>

#  include <expected>
#  include <system_error>
XSL_SYS_NB

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
