#include "xsl/sys/def.h"
#include "xsl/sys/raw.h"

#include <fcntl.h>

#include <expected>
#include <system_error>
XSL_SYS_NB
std::expected<void, std::errc> set_blocking(int fd, bool blocking) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return std::unexpected{std::errc(errno)};
  }
  if (!blocking) [[likely]] {
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
XSL_SYS_NE
