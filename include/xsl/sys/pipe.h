#pragma once
#ifndef XSL_SYS_PIPE
#  define XSL_SYS_PIPE
#  include "xsl/coro/lazy.h"
#  include "xsl/sys/def.h"
#  include "xsl/sys/io/dev.h"

#  include <fcntl.h>

#  include <utility>
SYS_NB

const size_t MAX_SINGLE_FWD_SIZE = 4096;
std::pair<sys::io::ReadDevice, sys::io::WriteDevice> pipe();
std::pair<sys::io::AsyncReadDevice, sys::io::AsyncWriteDevice> async_pipe(
    std::shared_ptr<sync::Poller>& poller);

coro::Lazy<void> splice(sys::io::AsyncReadDevice from, sys::io::AsyncWriteDevice to,
                        sys::io::AsyncReadDevice pipe_in, sys::io::AsyncWriteDevice pipe_out);
coro::Lazy<void> splice(sys::io::AsyncReadDevice from, sys::io::AsyncWriteDevice to,
                        std::shared_ptr<sync::Poller>& poller);
SYS_NE
#endif
