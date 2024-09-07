/**
 * @file pipe.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Pipe device
 * @version 0.12
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_PIPE
#  define XSL_SYS_PIPE
#  include "xsl/coro.h"
#  include "xsl/sys/def.h"
#  include "xsl/sys/dev.h"
#  include "xsl/sys/sync.h"

#  include <fcntl.h>

#  include <optional>
#  include <utility>
XSL_SYS_NB

using PipeReadDevice = RawDevice<In<byte>>;
using PipeWriteDevice = RawDevice<Out<byte>>;
using AsyncPipeReadDevice = RawAsyncDevice<In<byte>>;
using AsyncPipeWriteDevice = RawAsyncDevice<Out<byte>>;

const size_t MAX_SINGLE_FWD_SIZE = 4096;
/// @brief create a pipe
std::optional<std::pair<PipeReadDevice, PipeWriteDevice>> pipe();
/// @brief create a async pipe
std::optional<std::pair<AsyncPipeReadDevice, AsyncPipeWriteDevice>> async_pipe(
    std::shared_ptr<Poller>& poller);

/**
 * @brief splice data from a device to another device, one of the device must be a pipe
 *
 * @tparam From the source device
 * @tparam To the destination device
 * @param from the source device
 * @param to the destination device
 * @return Task<std::optional<std::errc>>
 */
template <AsyncRawDeviceLike From, AsyncRawDeviceLike To>
Task<std::optional<std::errc>> splice_single(From from, To to) {
  std::size_t offset = 0;
  do {
    ssize_t n = ::splice(from.raw(), nullptr, to.raw(), nullptr, MAX_SINGLE_FWD_SIZE,
                         SPLICE_F_MOVE | SPLICE_F_MORE | SPLICE_F_NONBLOCK);
    LOG5("recv n: {}", n);
    if (n > 0) {
      offset += n;
    } else if (n == 0) {
      if (offset != 0) {
        break;
      }
      co_return std::errc::no_message;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (offset != 0) {
        break;
      }
      if (!co_await from.poll_for_read()) {
        co_return std::errc::not_connected;
      }
    } else {
      co_return std::errc(errno);
    }
  } while (false);
  co_return std::nullopt;
}
/**
 * @brief splice data from a device to another device using a pipe
 *
 * @tparam From the source device
 * @tparam To the destination device
 * @param from the source device
 * @param to the destination device
 * @param pipe_in the pipe to read from
 * @param pipe_out the pipe to write to
 * @return Task<void>
 */
template <AsyncRawDeviceLike From, AsyncRawDeviceLike To>
Task<void> splice(From from, To to, AsyncPipeReadDevice pipe_in, AsyncPipeWriteDevice pipe_out) {
  splice_single(std::move(from), std::move(pipe_out)).detach(co_await coro::GetExecutor());
  splice_single(std::move(pipe_in), std::move(to)).detach(co_await coro::GetExecutor());
  co_return;
}
/**
 * @brief splice data from a device to another device using a pipe
 *
 * @tparam From the source device
 * @tparam To the destination device
 * @param from the source device
 * @param to the destination device
 * @param poller the poller
 * @return Task<void>
 */
template <AsyncRawDeviceLike From, AsyncRawDeviceLike To>
Task<void> splice(From from, To to, std::shared_ptr<Poller>& poller) {
  auto pipe = async_pipe(poller);
  if (!pipe) {
    co_return;
  }
  auto [pipe_in, pipe_out] = std::move(*pipe);
  co_return co_await splice(std::move(from), std::move(to), std::move(pipe_in),
                            std::move(pipe_out));
}
XSL_SYS_NE
#endif
