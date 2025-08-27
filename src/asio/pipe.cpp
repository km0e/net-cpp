/**
 * @file pipe.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Pipe utilities
 * @version 0.1.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <fcntl.h>
#include <sys/raw.h>
#include <unistd.h>
#include <xsl/asio/dev.h>
#include <xsl/asio/pipe.h>
#include <xsl/def.h>
#include <xsl/io/context.h>
#include <xsl/log.h>

#include <utility>
XSL_ASIO_NB

Expected<std::pair<AsyncPipeReadDevice, AsyncPipeWriteDevice>, errc> async_pipe(Context& ctx) {
  int fds[2];
  ENSEC(pipe2(fds, O_NONBLOCK | O_CLOEXEC) == 0);
  TRVEC(read, make_async_device<io::IOM_EVENTS::IN>(ctx, RawOwner{fds[0]}, AsyncPipeTraits{}));
  TRVEC(write, make_async_device<io::IOM_EVENTS::OUT>(ctx, RawOwner{fds[1]}, AsyncPipeTraits{}));
  return {std::make_pair(std::move(read), std::move(write))};
}

XSL_ASIO_NE
