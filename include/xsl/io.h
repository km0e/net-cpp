/**
 * @file io.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
 * @date 2024-08-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_IO
#  define XSL_IO
#  include "xsl/def.h"
#  include "xsl/io/byte.h"
#  include "xsl/io/def.h"
#  include "xsl/io/splice.h"

#  include <fcntl.h>
XSL_NB
using io::AIOTraits;
using io::IOTraits;

using io::Block;
using io::splice;
using io::splice_once;
using io::WriteFileHint;

XSL_NE
#endif
