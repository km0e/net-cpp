#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_HELPER_FILE_H_
#  define _XSL_NET_TRANSPORT_TCP_HELPER_FILE_H_
#  include "xsl/net/transport/tcp/context.h"
#  include "xsl/net/transport/tcp/helper/def.h"
TCP_HELPER_NAMESPACE_BEGIN
class SendFile : public tcp::SendTaskNode {
public:
  SendFile(string&& path);
  ~SendFile();
  tcp::SendResult exec(tcp::SendContext& ctx) override;

protected:
  list<string> path_buffer;
};
TCP_HELPER_NAMESPACE_END
#endif
