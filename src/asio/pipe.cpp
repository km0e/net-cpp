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
#include <xsl/asio/io.h>
#include <xsl/asio/pipe.h>
#include <xsl/def.h>
#include <xsl/log.h>

#include <utility>
XSL_ASIO_NB

Expected<std::pair<AsyncPipeReadDevice, AsyncPipeWriteDevice>, errc> async_pipe(IOContext& ctx) {
  int fds[2];
  ENSEC(pipe2(fds, O_NONBLOCK | O_CLOEXEC) == 0);
  auto read = AsyncPipeReadDevice();
  ENSEC(init_async_device(read, RawOwner{fds[0]}, ctx));
  auto write = AsyncPipeWriteDevice();
  ENSEC(init_async_device(write, RawOwner{fds[1]}, ctx));
  return {std::make_pair(std::move(read), std::move(write))};
}

XSL_ASIO_NE
