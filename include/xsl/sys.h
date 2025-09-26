/**
 * @file sys.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief System utilities
 * @version 0.1.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS
#  define XSL_SYS
#  include <xsl/def.h>
#  include <xsl/sys/io.h>
#  include <xsl/sys/net.h>
#  include <xsl/sys/net/def.h>
#  include <xsl/sys/net/error.h>
#  include <xsl/sys/net/gai.h>
#  include <xsl/sys/net/sockaddr.h>
#  include <xsl/sys/net/socket.h>
#  include <xsl/sys/net/utils.h>
#  include <xsl/sys/raw.h>

XSL_NB

namespace sys {
  using sys::current_ec;
  using sys::filter_interrupt;
}  // namespace sys
using sys::RawHandle;
using sys::RawOwner;
XSL_NE
#endif
