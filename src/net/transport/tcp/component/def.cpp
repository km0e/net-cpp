#include "xsl/net/transport/tcp/component/def.h"
TCP_COMPONENTS_NAMESPACE_BEGIN

SendContext::SendContext(int sfd, SendTasks& tasks) : sfd(sfd), tasks(tasks) {}
SendContext::~SendContext() {}
RecvContext::RecvContext(int sfd, RecvTasks& tasks) : sfd(sfd), tasks(tasks), iter(tasks.begin()) {}
RecvContext::~RecvContext() {}
SendTasksProxy::SendTasksProxy() : tasks() {}
SendTasksProxy::~SendTasksProxy() {}
SendResult SendTasksProxy::exec(int fd) {
  SendContext ctx(fd, this->tasks);
  bool res = true;
  while (!this->tasks.empty()) {
    auto res = this->tasks.front()->exec(ctx);
    if (res.is_ok()) {
      if (!res.unwrap()) {
        res = false;
        break;
      }
      this->tasks.pop_front();
    } else {
      return {SendError::Unknown};
    }
  }
  return {res};
}
TCP_COMPONENTS_NAMESPACE_END
