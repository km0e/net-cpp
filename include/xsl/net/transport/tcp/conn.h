#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_CONN_H_
#  define _XSL_NET_TRANSPORT_TCP_CONN_H_
#  include "xsl/net/sync.h"
#  include "xsl/net/sync/poller.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/wheel.h"

#  include <spdlog/spdlog.h>
TCP_NAMESPACE_BEGIN
enum class HandleHint {
  NONE = 0,
  // Hint that the param data is a pointer to a string that should be sent
  WRITE = 2,
};

class HandleState {
public:
  HandleState();
  HandleState(IOM_EVENTS events, HandleHint hint);
  ~HandleState();
  IOM_EVENTS events;
  HandleHint hint;
};

// Handler is a function that takes an int and a forward_list of TaskNode and returns a HandleState
// i is the cmd, 0 for read, 1 for write
// while return a HandleState
// - first check hint
//    - hint is WRITE and i is 0, send data
//    - hint is READ and i is 1, read data
template <class T>
concept TcpHandler = move_constructible<T> && requires(T t, int fd, IOM_EVENTS events) {
  { t.send(fd) } -> same_as<sync::PollHandleHint>;
  { t.recv(fd) } -> same_as<sync::PollHandleHint>;
  { t.other(fd, events) } -> same_as<sync::PollHandleHint>;
};

template <class T, class H>
concept TcpHandlerGenerator = TcpHandler<H> && requires(T t, H h) {
  { t() } -> same_as<H>;
};

template <TcpHandler H>
class TcpConn {
public:
  TcpConn(int fd, H&& handler) : fd(fd), events(IOM_EVENTS::IN), handler(move(handler)) {}

  TcpConn(TcpConn&&) = delete;
  TcpConn(const TcpConn&) = delete;
  ~TcpConn() {
    if (this->fd != -1) {
      close(this->fd);
    }
  }
  sync::PollHandleHint operator()(int fd, IOM_EVENTS events) {
    sync::PollHandleHint hint{};
    if (events == IOM_EVENTS::IN) {
      hint = handler.recv(fd);
    } else if (events == IOM_EVENTS::OUT) {
      hint = handler.send(fd);
    } else {
      hint = handler.other(fd, events);
    }
    return hint;
  }

private:
  int fd;
  IOM_EVENTS events;
  H handler;
};
TCP_NAMESPACE_END

#endif
