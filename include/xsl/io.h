/**
 * @file io.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.2.0
 * @date 2024-08-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_IO
#  define XSL_IO
#  include <fcntl.h>
#  include <xsl/def.h>
#  include <xsl/io/buf.h>
#  include <xsl/io/def.h>
#  include <xsl/io/ext.h>
XSL_NB

using io::Buffer;
using io::FixedBuffer;
using io::WriteFileHint;
using IOResult = io::Result;

XSL_NE
#endif
