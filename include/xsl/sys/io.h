/**
 * @file io.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief IO utilities
 * @version 0.1.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_IO
#  define XSL_SYS_IO
#  include <fcntl.h>
#  include <sys/sendfile.h>
#  include <unistd.h>
#  include <xsl/byte.h>
#  include <xsl/io/def.h>
#  include <xsl/io/ext.h>
#  include <xsl/log.h>
#  include <xsl/sys/def.h>
#  include <xsl/sys/io/context.h>
XSL_SYS_NB

XSL_SYS_NE
#endif
