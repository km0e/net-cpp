/**
 * @file resolve.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/sys/net/def.h"
#include "xsl/sys/net/resolve.h"
XSL_SYS_NET_NB
ResolveFlag operator|(ResolveFlag lhs, ResolveFlag rhs) {
  return static_cast<ResolveFlag>(static_cast<int>(lhs) | static_cast<int>(rhs));
}
XSL_SYS_NET_NE
