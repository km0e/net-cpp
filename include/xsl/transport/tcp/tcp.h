#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_H_
#  define _XSL_NET_TRANSPORT_TCP_H_

#  define TCP_NAMESPACE_BEGIN namespace xsl::transport::tcp {
#  define TCP_NAMESPACE_END }
#  include <xsl/sync/poller.h>
#  include <xsl/utils/wheel/wheel.h>
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

class TaskNode {
public:
  // - recv : return true if the task is done(recv all data)
  //          return false if the task is done, but not recv all data, will call next task
  //          if all tasks are done, but not recv all data, will call recv again
  // - send : return true if the task is done(send all data)
  virtual bool exec(int fd) = 0;
  virtual ~TaskNode() = default;
};
using Tasks = wheel::forward_list<wheel::unique_ptr<TaskNode>>;
class HandleConfig {
public:
  HandleConfig();
  HandleConfig(HandleConfig&&) = default;
  ~HandleConfig();
  Tasks recv_tasks;
};

// Handler is a function that takes an int and a forward_list of TaskNode and returns a HandleState
// i is the cmd, 0 for read, 1 for write
// while return a HandleState
// - first check hint
//    - hint is WRITE and i is 0, send data
//    - hint is READ and i is 1, read data
template <class T>
concept Handler = requires(T t, Tasks& hl) {
  { t.init() } -> wheel::same_as<HandleConfig>;
  { t.send(hl) } -> wheel::same_as<HandleState>;
  { t.recv(hl) } -> wheel::same_as<HandleState>;
};

template <class T, class H>
concept HandlerGenerator = Handler<H> && requires(T t, H h) {
  { t() } -> wheel::same_as<H>;
};
TCP_NAMESPACE_END
#endif
