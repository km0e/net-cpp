#include "xsl/coro/lazy.h"
#include "xsl/coro/task.h"
#include "xsl/logctl.h"
#include "xsl/sys/def.h"
#include "xsl/sys/pipe.h"

#include <fcntl.h>
#include <unistd.h>

#include <utility>
SYS_NB
std::pair<io::Device<feature::In>, io::Device<feature::Out>> pipe() {
  int fds[2];
  if (pipe2(fds, O_NONBLOCK) == -1) {
    LOG2("Failed to create pipe, err: {}", strerror(errno));
    return {io::Device<feature::In>(-1), io::Device<feature::Out>(-1)};
  }
  return {io::Device<feature::In>(fds[0]), io::Device<feature::Out>(fds[1])};
}

std::pair<io::AsyncDevice<feature::In>, io::AsyncDevice<feature::Out>> async_pipe(
    std::shared_ptr<sync::Poller>& poller) {
  int fds[2];
  if (pipe2(fds, O_NONBLOCK) == -1) {
    LOG2("Failed to create pipe, err: {}", strerror(errno));
    return {io::AsyncDevice<feature::In>(-1, nullptr), io::AsyncDevice<feature::Out>(-1, nullptr)};
  }
  auto read_sem = std::make_shared<coro::CountingSemaphore<1>>();
  auto write_sem = std::make_shared<coro::CountingSemaphore<1>>();
  poller->add(fds[0], sync::IOM_EVENTS::IN | sync::IOM_EVENTS::ET,
              sync::PollCallback<sync::IOM_EVENTS::IN>{read_sem});
  poller->add(fds[1], sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
              sync::PollCallback<sync::IOM_EVENTS::OUT>{write_sem});
  return {io::AsyncDevice<feature::In>(fds[0], read_sem), io::AsyncDevice<feature::Out>(fds[1], write_sem)};
}

SYS_NE
