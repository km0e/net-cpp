#include "xsl/sys/net/def.h"
#include "xsl/sys/net/resolve.h"
SYS_NET_NB
ResolveFlag operator|(ResolveFlag lhs, ResolveFlag rhs) {
  return static_cast<ResolveFlag>(static_cast<int>(lhs) | static_cast<int>(rhs));
}
SYS_NET_NE
