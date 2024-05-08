#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_CONN_H_
#  define _XSL_NET_TRANSPORT_TCP_CONN_H_
#  include <spdlog/spdlog.h>
#  include <xsl/transport/tcp/conn.h>
#  include <xsl/transport/tcp/tcp.h>

#  include "xsl/utils/wheel/wheel.h"
TCP_NAMESPACE_BEGIN

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
  Tasks recv_tasks;
  Tasks send_tasks;
  //   std::list < wheel::string send_buffer;
};
template <Handler H>
TcpConn<H>::TcpConn(int fd, wheel::shared_ptr<sync::Poller> poller, H&& handler)
    : fd(fd), poller(poller), handler(handler) {
  auto cfg = handler.init();
  this->recv_tasks.splice_after(this->recv_tasks.before_begin(), cfg.recv_tasks);
}

template <Handler H>
TcpConn<H>::~TcpConn() {
  close(this->fd);
}

template <Handler H>
sync::IOM_EVENTS TcpConn<H>::send() {
  spdlog::trace("[TcpConn::send]");
  Tasks hl;
  auto state = this->handler.send(hl);
  // move new tasks to this->tasks tail
  hl.splice_after(hl.before_begin(), this->send_tasks);
  this->send_tasks = std::move(hl);
  while (true) {
    if (this->send_tasks.front()->exec(this->fd)) {
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
  while (true) {
    for (auto& task : this->recv_tasks) {
      if (task->exec(this->fd)) {
        break;
      }
    }
  }
  auto state = this->handler.recv(this->recv_tasks);
  return state.events;
}
TCP_NAMESPACE_END

#endif
