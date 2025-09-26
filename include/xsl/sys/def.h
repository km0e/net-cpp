/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.1
 * @date 2024-08-31
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_DEF
#  define XSL_SYS_DEF
#  define XSL_SYS_NB \
    XSL_NB           \
    namespace sys {
#  define XSL_SYS_NE \
    }                \
    XSL_NE
#  include <fcntl.h>
#  include <xsl/io/def.h>
XSL_SYS_NB

#  ifdef XSL_UNDERLYING_IO_URING
#  else
#    define _XSL_UNDERLYING_EPOLL
#  endif

using RawHandle = int;

enum class DeviceAttribute {
  NonBlocking = O_NONBLOCK,
  CloseOnExec = O_CLOEXEC,
};

XSL_SYS_NE
#endif
