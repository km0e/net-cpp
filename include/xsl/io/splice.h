#pragma once
#ifndef XSL_IO_SPLICE
#  define XSL_IO_SPLICE
#  include "xsl/coro/lazy.h"
#  include "xsl/io/def.h"
#  include "xsl/sys/io/dev.h"

XSL_IO_NB
coro::Lazy<void> splice(sys::io::AsyncDevice<feature::In<std::byte>> from,
                        sys::io::AsyncDevice<feature::Out<std::byte>> to, std::string buffer);
XSL_IO_NE
#endif
