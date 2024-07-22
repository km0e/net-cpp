#include "xsl/logctl.h"
#include "xsl/net/transport/tcp/def.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>

TCP_NB

bool set_keep_alive(int fd, bool keep_alive) {
  int opt = keep_alive ? 1 : 0;
  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt))) {
    LOG2("Failed to set keep alive");
    return false;
  }
  return true;
}

TCP_NE
