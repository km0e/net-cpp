/**
 * @file pipe.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Pipe utilities
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/logctl.h"
#include "xsl/sys/def.h"
#include "xsl/sys/pipe.h"
#include "xsl/sys/sync.h"

#include <fcntl.h>
#include <sys/raw.h>
#include <unistd.h>

#include <utility>
XSL_SYS_NB
std::optional<std::pair<PipeReadDevice, PipeWriteDevice>> pipe() {
  int fds[2];
  if (pipe2(fds, O_NONBLOCK) == -1) {
    LOG2("Failed to create pipe, err: {}", strerror(errno));
    return std::nullopt;
  }
  return std::make_pair(PipeReadDevice{fds[0]}, PipeWriteDevice{fds[1]});
}

std::optional<std::pair<AsyncPipeReadDevice, AsyncPipeWriteDevice>> async_pipe(Poller& poller) {
  int fds[2];
  if (pipe2(fds, O_NONBLOCK | O_CLOEXEC) == -1) {
    LOG2("Failed to create pipe, err: {}", strerror(errno));
    return std::nullopt;
  }
  auto [read_signal] = poll_by_signal<DefaultPollTraits>(poller, fds[0], IOM_EVENTS::IN);
  auto [write_signal] = poll_by_signal<DefaultPollTraits>(poller, fds[1], IOM_EVENTS::OUT);
  return std::make_pair(AsyncPipeReadDevice{fds[0], std::move(read_signal)},
                        AsyncPipeWriteDevice{fds[1], std::move(write_signal)});
}

XSL_SYS_NE
