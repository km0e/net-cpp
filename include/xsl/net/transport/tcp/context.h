#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_CONTEXT_H_
#  define _XSL_NET_TRANSPORT_TCP_CONTEXT_H_
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/wheel.h"
TCP_NAMESPACE_BEGIN
enum class SendError {
  UNKNOWN,
};
string_view to_string(SendError err);
using SendResult = Result<bool, SendError>;
enum class RecvError {
  UNKNOWN,
  RECV_EOF,
};
string_view to_string(RecvError err);
using RecvResult = Result<bool, RecvError>;
class SendContext;
class RecvContext;
class SendTaskNode {
public:
  // - send : return true if the task is done, and will call next task
  //          return false if the task is done, but can't call next task
  virtual SendResult exec(SendContext& ctx) = 0;
  virtual ~SendTaskNode() = default;
};
class RecvTaskNode {
public:
  // - recv : return true if the task is done, and will call next task
  //          return false if the task is done, but can't call next task
  //          if all tasks are done, but not recv all data, will call recv again
  virtual RecvResult exec(RecvContext& ctx) = 0;
  virtual ~RecvTaskNode() = default;
};
using SendTasks = forward_list<unique_ptr<SendTaskNode>>;
using RecvTasks = forward_list<unique_ptr<RecvTaskNode>>;
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
