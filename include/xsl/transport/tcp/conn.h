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
concept Handler = wheel::move_constructible<T> && requires(T t, SendTasks& sts, RecvTasks& rts) {
  { t.init() } -> wheel::same_as<HandleConfig>;
  { t.send(sts) } -> wheel::same_as<HandleState>;
  { t.recv(rts) } -> wheel::same_as<HandleState>;
};

template <class T, class H>
concept HandlerGenerator = Handler<H> && requires(T t, H h) {
  { t() } -> wheel::same_as<H>;
};

template <Handler H>
class TcpConn {
public:
  static wheel::unique_ptr<TcpConn<H>> make_tcp_conn(int fd, wheel::shared_ptr<sync::Poller> poller,
                                                     H&& handler);
  TcpConn(TcpConn&&);
  // Don't directly use the constructor, use make_tcp_conn instead
  TcpConn(int fd, wheel::shared_ptr<sync::Poller> poller, H&& handler);
  ~TcpConn();
  void send();
  void recv();
  bool valid();

private:
  int fd;
  wheel::shared_ptr<sync::Poller> poller;
  H handler;
  RecvTasks recv_tasks;
  SendTasks send_tasks;
  sync::IOM_EVENTS events;
  //   std::list < wheel::string send_buffer;
};
template <Handler H>
wheel::unique_ptr<TcpConn<H>> TcpConn<H>::make_tcp_conn(int fd,
                                                        wheel::shared_ptr<sync::Poller> poller,
                                                        H&& handler) {
  auto conn = wheel::make_unique<TcpConn<H>>(fd, poller, wheel::move(handler));
  // we should pass the unique_ptr to the lambda, otherwise the conn will be deleted or moved
  poller->subscribe(fd, sync::IOM_EVENTS::IN,
                    [conn = conn.get()](int fd, sync::IOM_EVENTS events) { conn->recv(); });
  return conn;
}
template <Handler H>
TcpConn<H>::TcpConn(int fd, wheel::shared_ptr<sync::Poller> poller, H&& handler)
    : fd(fd), poller(poller), handler(wheel::move(handler)), events(sync::IOM_EVENTS::IN) {
  auto cfg = this->handler.init();
  this->recv_tasks.splice_after(this->recv_tasks.before_begin(), cfg.recv_tasks);
}
template <Handler H>
TcpConn<H>::TcpConn(TcpConn&& other)
    : fd(other.fd),
      poller(other.poller),
      handler(wheel::move(other.handler)),
      recv_tasks(wheel::move(other.recv_tasks)),
      send_tasks(wheel::move(other.send_tasks)),
      events(other.events) {
  other.events = sync::IOM_EVENTS::NONE;
  other.fd = -1;
}

template <Handler H>
TcpConn<H>::~TcpConn() {
  if (this->events != sync::IOM_EVENTS::NONE) {
    this->poller->unregister(this->fd);
    close(this->fd);
  };
}

template <Handler H>
bool TcpConn<H>::valid() {
  return this->events != sync::IOM_EVENTS::NONE;
}
template <Handler H>
void TcpConn<H>::send() {
  SPDLOG_TRACE("start send");
  SendTasks hl;
  auto state = this->handler.send(hl);
  // move new tasks to this->tasks tail
  hl.splice_after(hl.before_begin(), this->send_tasks);
  this->send_tasks = std::move(hl);
  SendContext ctx(this->fd, this->send_tasks);
  while (!this->send_tasks.empty()) {
    if (this->send_tasks.front()->exec(ctx)) {
      this->send_tasks.pop_front();
    } else {
      break;
    }
  }
  if (state.events != this->events) {
    this->poller->modify(this->fd, state.events);
  }
  this->events = state.events;
}
template <Handler H>
void TcpConn<H>::recv() {
  SPDLOG_TRACE("start recv");
  if (this->recv_tasks.empty()) {
    SPDLOG_ERROR("No recv task found");
    this->events = sync::IOM_EVENTS::NONE;
    this->poller->unregister(this->fd);
  }
  RecvContext ctx(this->fd, this->recv_tasks);
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
          this->events = sync::IOM_EVENTS::NONE;
          this->poller->unregister(this->fd);
          close(this->fd);
          return;
        case RecvError::UNKNOWN:
          SPDLOG_ERROR("recv error");
          this->events = sync::IOM_EVENTS::NONE;
          this->poller->unregister(this->fd);
          close(this->fd);
          return;
        default:
          break;
      }
    }
  }
  SPDLOG_DEBUG("recv all data");
  auto state = this->handler.recv(this->recv_tasks);
  if ((state.events & sync::IOM_EVENTS::OUT) == sync::IOM_EVENTS::OUT) {
    this->send();
  } else if (state.events != this->events) {
    this->poller->modify(this->fd, state.events);
    this->events = state.events;
  }
  SPDLOG_TRACE("recv done");
}
TCP_NAMESPACE_END

#endif
