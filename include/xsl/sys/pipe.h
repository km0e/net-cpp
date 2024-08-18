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
using PipeReadDevice = RawDevice<feature::In<impl_dev::PipeDeviceTraits>>;
using PipeWriteDevice = RawDevice<feature::Out<impl_dev::PipeDeviceTraits>>;
using AsyncPipeReadDevice = AsyncRawDevice<feature::In<impl_dev::PipeDeviceTraits>>;
using AsyncPipeWriteDevice = AsyncRawDevice<feature::Out<impl_dev::PipeDeviceTraits>>;

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
  while (true) {
    ssize_t n = ::splice(from.raw(), nullptr, to.raw(), nullptr, MAX_SINGLE_FWD_SIZE,
                         SPLICE_F_MOVE | SPLICE_F_MORE | SPLICE_F_NONBLOCK);
    LOG5("recv n: {}", n);
    if (n == 0) {
      co_return std::nullopt;
    } else if (n == -1) {
      if (errno == EAGAIN) {
        LOG5("no data");
        if (!co_await from.sem()) {
          LOG5("from {} is invalid", from.raw());
          co_return std::nullopt;
        }
      } else {
        LOG2("Failed to recv data, err : {}", strerror(errno));
        co_return std::errc(errno);
      }
    }
    LOG5("fwd: recv {} bytes", n);
  }
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
Lazy<void, Executor> splice(From from, To to, AsyncPipeReadDevice pipe_in,
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
Lazy<void, Executor> splice(From from, To to, std::shared_ptr<Poller>& poller) {
  auto [pipe_in, pipe_out] = async_pipe(poller);
  return splice<Executor>(std::move(from), std::move(to), std::move(pipe_in), std::move(pipe_out));
}
XSL_SYS_NE
#endif
