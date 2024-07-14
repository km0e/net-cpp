#pragma once
#ifndef XSL_NET_TRANSPORT_TCP_HELPER_FILE
#  define XSL_NET_TRANSPORT_TCP_HELPER_FILE
#  include "xsl/net/transport/tcp/component/def.h"

#  include <list>
TCP_COMPONENTS_NB
class SendFile : public SendTaskNode {
public:
  SendFile(std::string&& path);
  ~SendFile();
  std::expected<bool, RecvError> exec(SendContext& ctx) override;

protected:
  std::list<std::string> path_buffer;
};
TCP_COMPONENTS_NE
#endif
