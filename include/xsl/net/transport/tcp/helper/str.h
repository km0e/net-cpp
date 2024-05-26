#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_HELPER_STR_H_
#  define _XSL_NET_TRANSPORT_TCP_HELPER_STR_H_
#  include "xsl/net/transport/tcp/context.h"
#  include "xsl/net/transport/tcp/helper/def.h"
TCP_HELPER_NAMESPACE_BEGIN
class SendString : public tcp::SendTaskNode {
public:
  SendString(string&& data);
  SendString(SendString&&) = default;
  ~SendString();
  tcp::SendResult exec(tcp::SendContext& ctx) override;

private:
  list<string> data_buffer;
};
const size_t MAX_SINGLE_RECV_SIZE = 1024;
class RecvString : public tcp::RecvTaskNode {
public:
  RecvString(RecvString&&) = default;
  RecvString(string& data);
  ~RecvString();
  tcp::RecvResult exec(tcp::RecvContext& ctx) override;

private:
  string& data_buffer;
};
TCP_HELPER_NAMESPACE_END
#endif
