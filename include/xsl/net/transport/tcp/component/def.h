#pragma once
#ifndef XSL_NET_TRANSPORT_TCP_HELPER_DEF
#  define XSL_NET_TRANSPORT_TCP_HELPER_DEF

#  define TCP_COMPONENTS_NB namespace xsl::net::transport::tcp::component {
#  define TCP_COMPONENTS_NE }
#  include "xsl/net/transport/tcp/stream.h"

#  include <expected>
#  include <forward_list>
TCP_COMPONENTS_NB

class SendContext;
class RecvContext;
class SendTaskNode {
public:
  // - send : return true if the task is done, and will call next task
  //          return false if the task is done, but can't call next task
  virtual std::expected<bool, RecvError> exec(SendContext& ctx) = 0;
  virtual ~SendTaskNode() = default;
};
class RecvTaskNode {
public:
  // - recv : return true if the task is done, and will call next task
  //          return false if the task is done, but can't call next task
  //          if all tasks are done, but not recv all data, will call recv again
  virtual std::expected<bool, RecvErrorCategory> exec(RecvContext& ctx) = 0;
  virtual ~RecvTaskNode() = default;
};
using SendTasks = std::forward_list<std::unique_ptr<SendTaskNode>>;
using RecvTasks = std::forward_list<std::unique_ptr<RecvTaskNode>>;
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
class SendTasksProxy {
public:
  SendTasksProxy();
  SendTasksProxy(SendTasksProxy&&) = default;
  SendTasksProxy& operator=(SendTasksProxy&&) = default;
  SendTasksProxy(SendTasksProxy&) = delete;
  SendTasksProxy& operator=(SendTasksProxy&) = delete;
  ~SendTasksProxy();
  std::expected<bool, RecvErrorCategory> exec(int fd);
  SendTasks tasks;
};
TCP_COMPONENTS_NE
#endif
