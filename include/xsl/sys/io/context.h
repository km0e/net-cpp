/**
 * @file context.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Context for I/O operations
 * @version 0.1.2
 * @date 2025-06-02
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_SYS_IO_CONTEXT
#  define XSL_SYS_IO_CONTEXT
#  include <sys/epoll.h>
#  include <xsl/error.h>
#  include <xsl/io/def.h>
#  include <xsl/sync.h>
#  include <xsl/sys/def.h>

#  ifdef XSL_UNDERLYING_IO_URING
#  else
#    include <xsl/sys/io/context/epoll.h>

#  endif

XSL_SYS_NB

XSL_SYS_NE
#endif
