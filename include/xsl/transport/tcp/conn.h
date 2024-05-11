#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_CONN_H_
#  define _XSL_NET_TRANSPORT_TCP_CONN_H_
#  include <spdlog/spdlog.h>
#  include <xsl/sync/poller.h>
#  include <xsl/transport/tcp/conn.h>
#  include <xsl/transport/tcp/context.h>
#  include <xsl/transport/tcp/def.h>

#  include "xsl/utils/wheel/wheel.h"
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
concept Handler = requires(T t, SendTasks& sts, RecvTasks& rts) {
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
  TcpConn(TcpConn&&) = default;
  TcpConn(int fd, wheel::shared_ptr<sync::Poller> poller, H&& handler);
  ~TcpConn();
  sync::IOM_EVENTS send();
  sync::IOM_EVENTS recv();

private:
  int fd;
  wheel::shared_ptr<sync::Poller> poller;
  H handler;
  RecvTasks recv_tasks;
  SendTasks send_tasks;
  //   std::list < wheel::string send_buffer;
};
template <Handler H>
TcpConn<H>::TcpConn(int fd, wheel::shared_ptr<sync::Poller> poller, H&& handler)
    : fd(fd), poller(poller), handler(handler) {
  auto cfg = this->handler.init();
  this->recv_tasks.splice_after(this->recv_tasks.before_begin(), cfg.recv_tasks);
}

template <Handler H>
TcpConn<H>::~TcpConn() {
  close(this->fd);
}

template <Handler H>
sync::IOM_EVENTS TcpConn<H>::send() {
  spdlog::trace("[TcpConn::send]");
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
  return state.events;
}
template <Handler H>
sync::IOM_EVENTS TcpConn<H>::recv() {
  spdlog::trace("[TcpConn::recv]");
  if (this->recv_tasks.empty()) {
    spdlog::error("[TcpConn::recv] No recv task found");
    return sync::IOM_EVENTS::NONE;
  }
  RecvContext ctx(this->fd, this->recv_tasks);
  while (true) {
    if (ctx.iter->get()->exec(ctx)) break;
    ctx.iter++;
    if (ctx.iter == this->recv_tasks.end()) {
      ctx.iter = this->recv_tasks.begin();
    }
  }
  spdlog::debug("[TcpConn::recv] recv all data");
  auto state = this->handler.recv(this->recv_tasks);
  return state.events;
}
TCP_NAMESPACE_END

#endif
