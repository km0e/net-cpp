/**
 * @file io.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.2
 * @date 2024-08-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_IO
#  define XSL_IO
#  include "xsl/def.h"
#  include "xsl/io/buf.h"
#  include "xsl/io/context.h"
#  include "xsl/io/def.h"
#  include "xsl/io/ext.h"

#  include <fcntl.h>
XSL_NB

using io::AsyncRead;
using io::AsyncReadWrite;
using io::AsyncWrite;
using io::Buffer;
using io::Poller;
using io::WriteFileHint;

XSL_NE
#endif
