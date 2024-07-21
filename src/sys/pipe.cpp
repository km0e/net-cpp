#include "xsl/coro/lazy.h"
#include "xsl/coro/task.h"
#include "xsl/logctl.h"
#include "xsl/sys/def.h"
#include "xsl/sys/io.h"
#include "xsl/sys/pipe.h"

#include <fcntl.h>
#include <unistd.h>

#include <utility>
SYS_NB
std::pair<io::ReadDevice, io::WriteDevice> pipe() {
  int fds[2];
  if (pipe2(fds, O_NONBLOCK) == -1) {
    ERROR("Failed to create pipe, err: {}", strerror(errno));
    return {io::ReadDevice(-1), io::WriteDevice(-1)};
  }
  return {io::ReadDevice(fds[0]), io::WriteDevice(fds[1])};
}

std::pair<io::AsyncReadDevice, io::AsyncWriteDevice> async_pipe(
    std::shared_ptr<sync::Poller>& poller) {
  int fds[2];
  if (pipe2(fds, O_NONBLOCK) == -1) {
    ERROR("Failed to create pipe, err: {}", strerror(errno));
    return {io::AsyncReadDevice(-1, nullptr), io::AsyncWriteDevice(-1, nullptr)};
  }
  auto read_sem = std::make_shared<coro::CountingSemaphore<1>>();
  auto write_sem = std::make_shared<coro::CountingSemaphore<1>>();
  poller->add(fds[0], sync::IOM_EVENTS::IN | sync::IOM_EVENTS::ET,
              sync::PollCallback<sync::IOM_EVENTS::IN>{read_sem});
  poller->add(fds[1], sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
              sync::PollCallback<sync::IOM_EVENTS::OUT>{write_sem});
  return {io::AsyncReadDevice(fds[0], read_sem), io::AsyncWriteDevice(fds[1], write_sem)};
}

static coro::Task<std::optional<std::errc>> splice_single(io::AsyncReadDevice from,
                                                          io::AsyncWriteDevice to) {
  while (true) {
    ssize_t n = ::splice(from.raw(), nullptr, to.raw(), nullptr, MAX_SINGLE_FWD_SIZE,
                         SPLICE_F_MOVE | SPLICE_F_MORE | SPLICE_F_NONBLOCK);
    DEBUG("recv n: {}", n);
    if (n == 0) {
      co_return std::nullopt;
    } else if (n == -1) {
      if (errno == EAGAIN) {
        DEBUG("no data");
        co_await from.sem();
        if (!from.is_valid()) {
          DEBUG("from {} is invalid", from.raw());
          co_return std::nullopt;
        }
      } else {
        ERROR("Failed to recv data, err : {}", strerror(errno));
        co_return std::errc(errno);
      }
    }
    DEBUG("fwd: recv {} bytes", n);
  }
  co_return std::nullopt;
}

inline coro::Lazy<void> splice(io::AsyncReadDevice from, io::AsyncWriteDevice to,
                               io::AsyncReadDevice pipe_in, io::AsyncWriteDevice pipe_out) {
  splice_single(std::move(from), std::move(pipe_out)).detach();
  splice_single(std::move(pipe_in), std::move(to)).detach();
  co_return;
}
coro::Lazy<void> splice(io::AsyncReadDevice from, io::AsyncWriteDevice to,
                        std::shared_ptr<sync::Poller>& poller) {
  auto [pipe_in, pipe_out] = async_pipe(poller);
  return splice(std::move(from), std::move(to), std::move(pipe_in), std::move(pipe_out));
}
SYS_NE
