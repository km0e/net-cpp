#include "xsl/transport/tcp/context.h"
TCP_NAMESPACE_BEGIN
wheel::string_view to_string(RecvError err) {
  switch (err) {
    case RecvError::UNKNOWN:
      return "UNKNOWN";
    case RecvError::RECV_EOF:
      return "RECV_EOF";
    default:
      return "UNKNOWN";
  }
}

SendContext::SendContext(int sfd, SendTasks& tasks) : sfd(sfd), tasks(tasks) {}
SendContext::~SendContext() {}
RecvContext::RecvContext(int sfd, RecvTasks& tasks) : sfd(sfd), tasks(tasks), iter(tasks.begin()) {}
RecvContext::~RecvContext() {}
TCP_NAMESPACE_END
