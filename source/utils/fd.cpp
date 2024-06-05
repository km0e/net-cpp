#include "xsl/utils/def.h"
#include "xsl/utils/fd.h"

#include <fcntl.h>
UTILS_NAMESPACE_BEGIN
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
UTILS_NAMESPACE_END
