#pragma once
#ifndef XSL_NET_TRANSPORT_TCP_HELPER
#  define XSL_NET_TRANSPORT_TCP_HELPER
#  include "xsl/net/transport/tcp/component/def.h"
#  include "xsl/net/transport/tcp/component/file.h"
#  include "xsl/net/transport/tcp/component/str.h"
#  include "xsl/net/transport/tcp/def.h"
TCP_NB
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
TCP_NE
#endif
