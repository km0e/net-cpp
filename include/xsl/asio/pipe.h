/**
 * @file pipe.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Async pipe device
 * @version 0.1.1
 * @date 2025-06-03
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_ASIO_PIPE
#  define XSL_ASIO_PIPE
#  include <xsl/asio/def.h>
#  include <xsl/asio/dev.h>
#  include <xsl/asio/io.h>
#  include <xsl/compose.h>
#  include <xsl/feature.h>
#  include <xsl/io.h>
XSL_ASIO_NB

const size_t MAX_SINGLE_FWD_SIZE = 4096;
struct AsyncPipeStorage {
  using poll_traits_type = sys::DefaultPollTraits;
};
XSL_ASIO_NE
XSL_ASIO_NB

using AsyncPipeReadDevice = shared_memory<
    DefaultEpollWrapper<LocalCompose<DirectAsyncReadWriteUtils, AsyncDeviceUtil, RawOwner,
                                     IOSignalStorage<IOM_EVENTS::IN>, AsyncPipeStorage>>>;
using AsyncPipeWriteDevice = shared_memory<
    DefaultEpollWrapper<LocalCompose<DirectAsyncReadWriteUtils, AsyncDeviceUtil, RawOwner,
                                     IOSignalStorage<IOM_EVENTS::OUT>, AsyncPipeStorage>>>;

/// @brief create a async pipe
std::expected<std::pair<AsyncPipeReadDevice, AsyncPipeWriteDevice>, errc> async_pipe(
    IOContext& ctx);

/**
 * @brief splice data from a device to another device, one of the device must be a pipe
 *
 * @tparam From the source device
 * @tparam To the destination device
 * @param from the source device
 * @param to the destination device
 * @return Task<std::optional<errc>>
 */
template <typename From, typename To>
Task<std::optional<errc>> splice(From from, To to) {
  std::size_t offset = 0;
  do {
    ssize_t n = ::splice(from->raw(), nullptr, to->raw(), nullptr, MAX_SINGLE_FWD_SIZE,
                         SPLICE_F_MOVE | SPLICE_F_MORE | SPLICE_F_NONBLOCK);
    log_debug("recv n: {}", n);
    if (n > 0) {
      offset += n;
    } else if (n == 0) {
      if (offset != 0) {
        break;
      }
      co_return errc::no_message;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (offset != 0) {
        break;
      }
      if (!co_await from->read_signal()) {
        co_return errc::not_connected;
      }
    } else {
      co_return errc(errno);
    }
  } while (true);
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
template <AsyncRead From, AsyncWrite To>
Task<void> splice_bidirectional(From from, To to, AsyncPipeReadDevice pipe_in,
                                AsyncPipeWriteDevice pipe_out) {
  co_yield splice(std::move(from), std::move(pipe_out));
  co_yield splice(std::move(pipe_in), std::move(to));
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
template <AsyncRead From, AsyncWrite To>
std::expected<Task<void>, errc> splice_bidirectional(From from, To to, IOContext& ctx) {
  TRVEC(pipe, async_pipe(ctx));
  auto [pipe_in, pipe_out] = std::move(pipe);
  return {splice_bidirectional(std::move(from), std::move(to), std::move(pipe_in),
                               std::move(pipe_out))};
}
XSL_ASIO_NE
#endif
