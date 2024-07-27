#include "xsl/coro/lazy.h"
#include "xsl/coro/task.h"
#include "xsl/logctl.h"
#include "xsl/sys/def.h"
#include "xsl/sys/pipe.h"

#include <fcntl.h>
#include <unistd.h>

#include <utility>
SYS_NB
std::pair<io::ReadDevice, io::WriteDevice> pipe() {
  int fds[2];
  if (pipe2(fds, O_NONBLOCK) == -1) {
    LOG2("Failed to create pipe, err: {}", strerror(errno));
    return {io::ReadDevice(-1), io::WriteDevice(-1)};
  }
  return {io::ReadDevice(fds[0]), io::WriteDevice(fds[1])};
}

std::pair<io::AsyncReadDevice, io::AsyncWriteDevice> async_pipe(
    std::shared_ptr<sync::Poller>& poller) {
  int fds[2];
  if (pipe2(fds, O_NONBLOCK) == -1) {
    LOG2("Failed to create pipe, err: {}", strerror(errno));
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

SYS_NE
