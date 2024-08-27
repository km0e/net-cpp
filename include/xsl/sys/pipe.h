/**
 * @file pipe.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Pipe device
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_PIPE
#  define XSL_SYS_PIPE
#  include "xsl/coro.h"
#  include "xsl/feature.h"
#  include "xsl/sys/def.h"
#  include "xsl/sys/raw.h"
#  include "xsl/sys/sync.h"

#  include <fcntl.h>

#  include <utility>
XSL_SYS_NB
namespace impl_dev {
  struct PipeDeviceTraits {
    using value_type = byte;
  };
}  // namespace impl_dev
using PipeReadDevice = RawDevice<In<impl_dev::PipeDeviceTraits>>;
using PipeWriteDevice = RawDevice<Out<impl_dev::PipeDeviceTraits>>;
using AsyncPipeReadDevice = AsyncRawDevice<In<impl_dev::PipeDeviceTraits>>;
using AsyncPipeWriteDevice = AsyncRawDevice<Out<impl_dev::PipeDeviceTraits>>;

const size_t MAX_SINGLE_FWD_SIZE = 4096;
/**
 * @brief create a pipe
 *
 * @return std::pair<PipeReadDevice, PipeWriteDevice>
 */
std::pair<PipeReadDevice, PipeWriteDevice> pipe();
/**
 * @brief create a async pipe
 *
 * @param poller
 * @return std::pair<AsyncPipeReadDevice, AsyncPipeWriteDevice>
 */
std::pair<AsyncPipeReadDevice, AsyncPipeWriteDevice> async_pipe(std::shared_ptr<Poller>& poller);

/**
 * @brief splice data from a device to another device, one of the device must be a pipe
 *
 * @tparam Executor the executor type
 * @tparam From the source device type
 * @tparam To the destination device type
 * @param from the source device
 * @param to the destination device
 * @return Task<std::optional<std::errc>, Executor>
 */
template <class Executor = coro::ExecutorBase, AsyncRawDeviceLike From, AsyncRawDeviceLike To>
Task<std::optional<std::errc>, Executor> splice_single(From from, To to) {
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
 * @tparam Executor the executor type
 * @tparam From the source device type
 * @tparam To the destination device type
 * @param from the source device
 * @param to the destination device
 * @param pipe_in the input pipe
 * @param pipe_out the output pipe
 * @return Lazy<void, Executor>
 */
template <class Executor = coro::ExecutorBase, AsyncRawDeviceLike From, AsyncRawDeviceLike To>
Task<void, Executor> splice(From from, To to, AsyncPipeReadDevice pipe_in,
                            AsyncPipeWriteDevice pipe_out) {
  splice_single<Executor>(std::move(from), std::move(pipe_out))
      .detach(co_await coro::GetExecutor<Executor>());
  splice_single<Executor>(std::move(pipe_in), std::move(to))
      .detach(co_await coro::GetExecutor<Executor>());
  co_return;
}
/**
 * @brief splice data from a device to another device using a pipe
 *
 * @tparam Executor
 * @tparam From
 * @tparam To
 * @param from
 * @param to
 * @param poller
 * @return Lazy<void, Executor>
 */
template <class Executor = coro::ExecutorBase, AsyncRawDeviceLike From, AsyncRawDeviceLike To>
Task<void, Executor> splice(From from, To to, std::shared_ptr<Poller>& poller) {
  auto [pipe_in, pipe_out] = async_pipe(poller);
  return splice<Executor>(std::move(from), std::move(to), std::move(pipe_in), std::move(pipe_out));
}
XSL_SYS_NE
#endif
