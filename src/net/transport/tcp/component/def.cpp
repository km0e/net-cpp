#include "xsl/net/transport/tcp/component/def.h"
#include <cstddef>
TCP_COMPONENTS_NAMESPACE_BEGIN

SendContext::SendContext(int sfd, SendTasks& tasks) : sfd(sfd), tasks(tasks) {}
SendContext::~SendContext() {}
RecvContext::RecvContext(int sfd, RecvTasks& tasks) : sfd(sfd), tasks(tasks), iter(tasks.begin()) {}
RecvContext::~RecvContext() {}
SendTasksProxy::SendTasksProxy() : tasks() {}
SendTasksProxy::~SendTasksProxy() {}
std::expected<bool, RecvErrorCategory> SendTasksProxy::exec(int fd) {
  // SendContext ctx(fd, this->tasks);
  // size_t sum = 0;
  // while (!this->tasks.empty()) {
  //   auto res = this->tasks.front()->exec(ctx);
  //   if (res.has_value()) {
  //     if (res.value() == 0) {
  //       sum = 0;
  //       break;
  //     }
  //     this->tasks.pop_front();
  //   } else {
  //     return std::unexpected{SendError::Unknown};
  //   }
  // }
  // return {sum};
  return {0};
}
TCP_COMPONENTS_NAMESPACE_END
