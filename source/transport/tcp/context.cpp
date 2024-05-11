#include "xsl/transport/tcp/context.h"
#include "xsl/transport/tcp/tcp.h"
TCP_NAMESPACE_BEGIN
SendContext::SendContext(int sfd, SendTasks& tasks) : sfd(sfd), tasks(tasks) {}
SendContext::~SendContext() {}
RecvContext::RecvContext(int sfd, RecvTasks& tasks) : sfd(sfd), tasks(tasks), iter(tasks.begin()) {}
RecvContext::~RecvContext() {}
TCP_NAMESPACE_END
