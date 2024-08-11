#pragma once
#ifndef XSL_SYS_PIPE
#  define XSL_SYS_PIPE
#  include "xsl/coro.h"
#  include "xsl/feature.h"
#  include "xsl/sys/def.h"
#  include "xsl/sys/io/dev.h"

#  include <fcntl.h>

#  include <utility>
XSL_SYS_NB

const size_t MAX_SINGLE_FWD_SIZE = 4096;
std::pair<sys::io::Device<feature::In<std::byte>>, sys::io::Device<feature::Out<std::byte>>> pipe();
std::pair<sys::io::AsyncDevice<feature::In<std::byte>>,
          sys::io::AsyncDevice<feature::Out<std::byte>>>
async_pipe(std::shared_ptr<sync::Poller>& poller);

template <class Executor = coro::ExecutorBase>
coro::Task<std::optional<std::errc>, Executor> splice_single(
    io::AsyncDevice<feature::In<std::byte>> from, io::AsyncDevice<feature::Out<std::byte>> to) {
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

template <class Executor = coro::ExecutorBase>
coro::Lazy<void, Executor> splice(io::AsyncDevice<feature::In<std::byte>> from,
                                  io::AsyncDevice<feature::Out<std::byte>> to,
                                  io::AsyncDevice<feature::In<std::byte>> pipe_in,
                                  io::AsyncDevice<feature::Out<std::byte>> pipe_out) {
  splice_single<Executor>(std::move(from), std::move(pipe_out))
      .detach(co_await coro::GetExecutor<Executor>());
  splice_single<Executor>(std::move(pipe_in), std::move(to))
      .detach(co_await coro::GetExecutor<Executor>());
  co_return;
}
template <class Executor = coro::ExecutorBase>
coro::Lazy<void, Executor> splice(io::AsyncDevice<feature::In<std::byte>> from,
                                  io::AsyncDevice<feature::Out<std::byte>> to,
                                  std::shared_ptr<sync::Poller>& poller) {
  auto [pipe_in, pipe_out] = async_pipe(poller);
  return splice<Executor>(std::move(from), std::move(to), std::move(pipe_in), std::move(pipe_out));
}
XSL_SYS_NE
#endif
