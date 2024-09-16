/**
 * @file raw.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/sys/def.h"
#include "xsl/sys/raw.h"

#include <fcntl.h>

#include <expected>
#include <system_error>
XSL_SYS_NB
std::expected<void, errc> set_blocking(int fd, bool blocking) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return std::unexpected{errc(errno)};
  }
  if (!blocking) [[likely]] {
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
XSL_SYS_NE
