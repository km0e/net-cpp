#pragma once
#ifndef XSL_IO_SPLICE
#  define XSL_IO_SPLICE
#  include "xsl/coro/lazy.h"
#  include "xsl/io/def.h"
#  include "xsl/sys/io.h"

XSL_IO_NB
coro::Lazy<void> splice(sys::io::AsyncReadDevice from, sys::io::AsyncWriteDevice to,
                        std::string buffer);
XSL_IO_NE
#endif
