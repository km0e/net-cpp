#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_CONN_H_
#  define _XSL_NET_TRANSPORT_TCP_CONN_H_
#  include "xsl/sync/sync.h"
#  include "xsl/transport/tcp/context.h"
#  include "xsl/transport/tcp/def.h"
#  include "xsl/wheel/wheel.h"

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
  HandleState(sync::IOM_EVENTS events, HandleHint hint);
  ~HandleState();
  sync::IOM_EVENTS events;
  HandleHint hint;
};

class HandleConfig {
public:
  HandleConfig();
  HandleConfig(HandleConfig&&) = default;
  ~HandleConfig();
  RecvTasks recv_tasks;
};

// Handler is a function that takes an int and a forward_list of TaskNode and returns a HandleState
// i is the cmd, 0 for read, 1 for write
// while return a HandleState
// - first check hint
//    - hint is WRITE and i is 0, send data
//    - hint is READ and i is 1, read data
template <class T>
concept TcpHandler = wheel::move_constructible<T> && requires(T t, SendTasks& sts, RecvTasks& rts) {
  { t.init() } -> wheel::same_as<HandleConfig>;
  { t.send(sts) } -> wheel::same_as<HandleState>;
  { t.recv(rts) } -> wheel::same_as<HandleState>;
};

template <class T, class H>
concept HandlerGenerator = TcpHandler<H> && requires(T t, H h) {
  { t() } -> wheel::same_as<H>;
};

template <TcpHandler H>
class TcpConn {
public:
  TcpConn(int fd, wheel::shared_ptr<sync::Poller> poller, H&& handler);
  TcpConn(TcpConn&&) = delete;
  TcpConn(const TcpConn&) = delete;
  ~TcpConn();
  void send(int fd, sync::IOM_EVENTS events);
  void recv(int fd, sync::IOM_EVENTS events);
  bool valid();

private:
  int fd;
  sync::IOM_EVENTS events;
  wheel::shared_ptr<sync::Poller> poller;
  RecvTasks recv_tasks;
  SendTasks send_tasks;
  H handler;
};
template <TcpHandler H>
TcpConn<H>::TcpConn(int fd, wheel::shared_ptr<sync::Poller> poller, H&& handler)
    : fd(fd),
      events(sync::IOM_EVENTS::IN),
      poller(poller),
      recv_tasks(),
      send_tasks(),
      handler(wheel::move(handler)) {
  poller->subscribe(fd, sync::IOM_EVENTS::IN,
                    [this](int fd, sync::IOM_EVENTS events) { this->recv(fd, events); });
  auto cfg = this->handler.init();
  this->recv_tasks.splice_after(this->recv_tasks.before_begin(), cfg.recv_tasks);
}

template <TcpHandler H>
TcpConn<H>::~TcpConn() {
  if (this->events != sync::IOM_EVENTS::NONE) {
    this->poller->unregister(this->fd);
  }
  if (this->fd != -1) {
    close(this->fd);
  }
}

template <TcpHandler H>
bool TcpConn<H>::valid() {
  return this->events != sync::IOM_EVENTS::NONE;
}
template <TcpHandler H>
void TcpConn<H>::send(int fd, sync::IOM_EVENTS events) {
  (void)events;
  SPDLOG_TRACE("start send");
  SendTasks hl;
  auto state = this->handler.send(hl);
  // move new tasks to this->tasks tail
  hl.splice_after(hl.before_begin(), this->send_tasks);
  this->send_tasks = std::move(hl);
  SendContext ctx(fd, this->send_tasks);
  while (!this->send_tasks.empty()) {
    if (this->send_tasks.front()->exec(ctx)) {
      this->send_tasks.pop_front();
    } else {
      break;
    }
  }
  if (state.events != this->events) {
    this->poller->modify(fd, state.events);
  }
  this->events = state.events;
}
template <TcpHandler H>
void TcpConn<H>::recv(int fd, sync::IOM_EVENTS events) {
  (void)events;
  SPDLOG_TRACE("start recv");
  if (this->recv_tasks.empty()) {
    SPDLOG_ERROR("No recv task found");
    // this->events = sync::IOM_EVENTS::NONE;
    // this->poller->unregister(fd);
    return ;
  }
  RecvContext ctx(fd, this->recv_tasks);
  while (true) {
    auto res = ctx.iter->get()->exec(ctx);
    if (res.is_ok()) {
      if (res.unwrap()) {
        break;
      }
      ctx.iter++;
      if (ctx.iter == this->recv_tasks.end()) {
        ctx.iter = this->recv_tasks.begin();
      }
    } else {
      switch (res.unwrap_err()) {
        case RecvError::RECV_EOF:
          SPDLOG_DEBUG("recv eof");
          this->poller->unregister(fd);
          this->events = sync::IOM_EVENTS::NONE;
          close(fd);
          fd = -1;
          return;
        case RecvError::UNKNOWN:
          SPDLOG_ERROR("recv error");
          this->poller->unregister(fd);
          this->events = sync::IOM_EVENTS::NONE;
          close(fd);
          fd = -1;
          return;
        default:
          break;
      }
    }
  }
  SPDLOG_DEBUG("recv all data");
  auto state = this->handler.recv(this->recv_tasks);
  if ((state.events & sync::IOM_EVENTS::OUT) == sync::IOM_EVENTS::OUT) {
    this->send(fd, sync::IOM_EVENTS::OUT);
  } else if (state.events != this->events) {
    this->poller->modify(fd, state.events);
    this->events = state.events;
  }
  SPDLOG_TRACE("recv done");
}
TCP_NAMESPACE_END

#endif
