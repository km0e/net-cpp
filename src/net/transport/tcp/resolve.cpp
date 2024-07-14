#include "xsl/net/transport/def.h"
#include "xsl/net/transport/resolve.h"
TRANSPORT_NB
ResolveFlag operator|(ResolveFlag lhs, ResolveFlag rhs) {
  return static_cast<ResolveFlag>(static_cast<int>(lhs) | static_cast<int>(rhs));
}
TRANSPORT_NE
