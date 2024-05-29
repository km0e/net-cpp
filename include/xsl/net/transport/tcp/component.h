#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_HELPER_H_
#  define _XSL_NET_TRANSPORT_TCP_HELPER_H_
#  include "xsl/net/transport/tcp/component/def.h"
#  include "xsl/net/transport/tcp/component/file.h"
#  include "xsl/net/transport/tcp/component/str.h"
#  include "xsl/net/transport/tcp/def.h"
TCP_NAMESPACE_BEGIN
using component::RecvContext;
using component::RecvTaskNode;
using component::RecvTasks;
using component::SendContext;
using component::SendFile;
using component::SendTaskNode;
using component::SendTasks;
using component::SendTasksProxy;
using component::TcpRecvString;
using component::TcpSendString;
TCP_NAMESPACE_END
#endif
