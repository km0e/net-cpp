#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_HELPER_PIPE_H_
#  define _XSL_NET_TRANSPORT_TCP_HELPER_PIPE_H_
#  include "xsl/net/transport/tcp/context.h"
#  include "xsl/net/transport/tcp/helper/def.h"

TCP_HELPER_NAMESPACE_BEGIN
const size_t MAX_SINGLE_PIPE_SIZE = 65535;

class SendPipe : public tcp::SendTaskNode {
public:
  SendPipe(int fd);
  ~SendPipe();
  tcp::SendResult exec(tcp::SendContext& ctx) override;

protected:
  int fd;
};
class RecvPipe : public tcp::RecvTaskNode {
public:
  RecvPipe(int fd);
  ~RecvPipe();
  tcp::RecvResult exec(tcp::RecvContext& ctx) override;

protected:
  int fd;
};
TCP_HELPER_NAMESPACE_END
#endif
