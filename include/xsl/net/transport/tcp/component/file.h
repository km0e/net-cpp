#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_HELPER_FILE_H_
#  define _XSL_NET_TRANSPORT_TCP_HELPER_FILE_H_
#  include "xsl/net/transport/tcp/component/def.h"

#  include <list>
TCP_COMPONENTS_NAMESPACE_BEGIN
class SendFile : public SendTaskNode {
public:
  SendFile(std::string&& path);
  ~SendFile();
  std::expected<bool, RecvError> exec(SendContext& ctx) override;

protected:
  std::list<std::string> path_buffer;
};
TCP_COMPONENTS_NAMESPACE_END
#endif
