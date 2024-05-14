#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_CONTEXT_H_
#  define _XSL_NET_TRANSPORT_TCP_CONTEXT_H_
#  include "xsl/transport/tcp/def.h"
#  include "xsl/wheel/wheel.h"
TCP_NAMESPACE_BEGIN
enum class RecvError {
  UNKNOWN,
  RECV_EOF,
};
wheel::string_view to_string(RecvError err);
using RecvResult = wheel::Result<bool, RecvError>;
class SendContext;
class RecvContext;
class SendTaskNode {
public:
  // - recv : return true if the task is done(recv all data)
  //          return false if the task is done, but not recv all data, will call next task
  //          if all tasks are done, but not recv all data, will call recv again
  // - send : return true if the task is done(send all data)
  virtual bool exec(SendContext& ctx) = 0;
  virtual ~SendTaskNode() = default;
};
class RecvTaskNode {
public:
  // - recv : return true if the task is done(recv all data)
  //          return false if the task is done, but not recv all data, will call next task
  //          if all tasks are done, but not recv all data, will call recv again
  // - send : return true if the task is done(send all data)
  virtual RecvResult exec(RecvContext& ctx) = 0;
  virtual ~RecvTaskNode() = default;
};
using SendTasks = wheel::forward_list<wheel::unique_ptr<SendTaskNode>>;
using RecvTasks = wheel::forward_list<wheel::unique_ptr<RecvTaskNode>>;
class SendContext {
public:
  SendContext(SendContext&&) = default;
  SendContext(int sfd, SendTasks& tasks);
  ~SendContext();
  int sfd;
  SendTasks& tasks;
};
class RecvContext {
public:
  RecvContext(RecvContext&&) = default;
  RecvContext(int sfd, RecvTasks& tasks);
  ~RecvContext();
  int sfd;
  RecvTasks& tasks;
  RecvTasks::iterator iter;
};
TCP_NAMESPACE_END
#endif
