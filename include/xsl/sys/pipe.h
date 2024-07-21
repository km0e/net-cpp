#pragma once
#ifndef XSL_SYS_PIPE
#  define XSL_SYS_PIPE
#  include "xsl/coro/lazy.h"
#  include "xsl/sys/def.h"
#  include "xsl/sys/io.h"

#  include <fcntl.h>

#  include <utility>
SYS_NB

const size_t MAX_SINGLE_FWD_SIZE = 4096;

std::pair<io::ReadDevice, io::WriteDevice> pipe();
std::pair<io::AsyncReadDevice, io::AsyncWriteDevice> async_pipe(
    std::shared_ptr<sync::Poller>& poller);

coro::Lazy<void> splice(io::AsyncReadDevice from, io::AsyncWriteDevice to,
                        io::AsyncReadDevice pipe_in, io::AsyncWriteDevice pipe_out);
coro::Lazy<void> splice(io::AsyncReadDevice from, io::AsyncWriteDevice to,
                        std::shared_ptr<sync::Poller>& poller);
SYS_NE
#endif
