#include "xsl/net/transport/tcp/context.h"
#include "xsl/net/transport/tcp/helper/def.h"
#include "xsl/net/transport/tcp/helper/pause.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

TCP_HELPER_NAMESPACE_BEGIN

SendPause::SendPause(bool& cont_send) : cont_send(cont_send) {}
SendPause::~SendPause() {}
// using splice to send data
tcp::SendResult SendPause::exec([[maybe_unused]] tcp::SendContext& ctx) {
  if (cont_send) {
    return {true};
  }
  cont_send = true;
  return {false};
}
RecvPause::RecvPause(bool& cont_recv) : cont_recv(cont_recv) {}
RecvPause::~RecvPause() {}
// using splice to receive data
tcp::RecvResult RecvPause::exec([[maybe_unused]] tcp::RecvContext& ctx) {
  if (cont_recv) {
    return {true};
  }
  cont_recv = true;
  return {false};
}
TCP_HELPER_NAMESPACE_END
