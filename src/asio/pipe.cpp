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
#include <xsl/asio/pipe.h>
#include <xsl/def.h>
#include <xsl/io/context.h>
#include <xsl/log.h>

#include <utility>
XSL_ASIO_NB

std::expected<std::pair<AsyncPipeReadDevice, AsyncPipeWriteDevice>, errc> async_pipe(Context& ctx) {
  int fds[2];
  if (pipe2(fds, O_NONBLOCK | O_CLOEXEC) == -1) {
    log_error("Failed to create pipe, err: {}", strerror(errno));
    return std::unexpected(errc(errno));
  }
  auto read = AsyncPipeReadDevice(RawOwner{fds[0]}, ctx, io::DefaultPollTraits{});
  auto write = AsyncPipeWriteDevice(RawOwner{fds[1]}, ctx, io::DefaultPollTraits{});
  return {std::make_pair(std::move(read), std::move(write))};
}

XSL_ASIO_NE
