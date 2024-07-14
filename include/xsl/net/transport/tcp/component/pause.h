// #pragma once
// #ifndef XSL_NET_TRANSPORT_TCP_HELPER_PAUSE
// #  define XSL_NET_TRANSPORT_TCP_HELPER_PAUSE
// #  include "xsl/net/transport/tcp/context.h"
// #  include "xsl/net/transport/tcp/helper/def.h"

// TCP_HELPER_NB
// class SendPause : public tcp::SendTaskNode {
// public:
//   SendPause(bool& cont_send);
//   ~SendPause();
//   tcp::SendResult exec(tcp::SendContext& ctx) override;

// protected:
//   bool& cont_send;
// };
// class RecvPause : public tcp::RecvTaskNode {
// public:
//   RecvPause(bool& cont_recv);
//   ~RecvPause();
//   tcp::RecvResult exec(tcp::RecvContext& ctx) override;

// protected:
//   bool& cont_recv;
// };
// TCP_HELPER_NE
// #endif
