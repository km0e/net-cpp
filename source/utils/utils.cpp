#include <fcntl.h>
#include <xsl/utils/utils.h>
bool set_non_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return false;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    return false;
  }
  return true;
}
