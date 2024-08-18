#include "xsl/logctl.h"
#include "xsl/sys/def.h"
#include "xsl/sys/pipe.h"
#include "xsl/sys/sync.h"

#include <fcntl.h>
#include <sys/raw.h>
#include <unistd.h>

#include <utility>
XSL_SYS_NB
std::pair<PipeReadDevice, PipeWriteDevice> pipe() {
  int fds[2];
  if (pipe2(fds, O_NONBLOCK) == -1) {
    LOG2("Failed to create pipe, err: {}", strerror(errno));
    return {{-1}, {-1}};
  }
  return {{fds[0]}, {fds[1]}};
}

std::pair<AsyncPipeReadDevice, AsyncPipeWriteDevice> async_pipe(std::shared_ptr<Poller>& poller) {
  int fds[2];
  if (pipe2(fds, O_NONBLOCK | O_CLOEXEC) == -1) {
    LOG2("Failed to create pipe, err: {}", strerror(errno));
    return {{nullptr, -1}, {nullptr, -1}};
  }
  auto read_sem = std::make_shared<CountingSemaphore<1>>();
  auto write_sem = std::make_shared<CountingSemaphore<1>>();
  poller->add(fds[0], PollForCoro<DefaultPollTraits, IOM_EVENTS::IN>{read_sem});
  poller->add(fds[1], PollForCoro<DefaultPollTraits, IOM_EVENTS::OUT>{write_sem});
  return {{read_sem, fds[0]}, {write_sem, fds[1]}};
}

XSL_SYS_NE
