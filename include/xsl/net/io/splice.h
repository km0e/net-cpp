#pragma once
#ifndef XSL_NET_IO_SPLICE
#  define XSL_NET_IO_SPLICE
#  include "xsl/coro/lazy.h"
#  include "xsl/net/io/def.h"
#  include "xsl/sys/io/dev.h"
XSL_NET_IO_NB
coro::Lazy<void> splice(sys::io::AsyncReadDevice from, sys::io::AsyncWriteDevice to,
                        std::string buffer);
XSL_NET_IO_NE
#endif
