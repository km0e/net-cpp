#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_SERVER_H_
#  define _XSL_NET_TRANSPORT_TCP_SERVER_H_
#  include "xsl/logctl.h"
#  include "xsl/net/sync.h"
#  include "xsl/net/sync/poller.h"
#  include "xsl/net/transport/tcp/conn.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/utils.h"

#  include <unistd.h>

#  include <memory>

TCP_NAMESPACE_BEGIN

template <TcpHandlerLike H, TcpHandlerGeneratorLike<H> HG>
class TcpServer {
public:
  TcpServer(TcpServer&&) = delete;
  TcpServer(std::shared_ptr<HG> handler_generator, TcpConnManagerConfig conn_manager_config)
      : handler_generator(handler_generator), tcp_conn_manager(conn_manager_config) {}
  ~TcpServer() {}
  PollHandleHint operator()(int fd, IOM_EVENTS events) {
    if ((events & IOM_EVENTS::IN) == IOM_EVENTS::IN) {
      sockaddr addr;
      socklen_t addr_len = sizeof(addr);
      int client_fd = ::accept(fd, &addr, &addr_len);
      if (client_fd == -1) {  // todo: handle error
        return {PollHandleHintTag::NONE};
      }
      if (!set_non_blocking(client_fd)) {
        WARNING("[TcpServer::<lambda::handler>] Failed to set non-blocking");
        close(client_fd);
        return {PollHandleHintTag::NONE};
      }
      auto handler = (*this->handler_generator)(client_fd);
      if (!handler) {
        WARNING("[TcpServer::<lambda::handler>] Failed to create handler");
        close(client_fd);
        return {PollHandleHintTag::NONE};
      }
      this->tcp_conn_manager.add(client_fd, std::move(handler));
      return {PollHandleHintTag::NONE};
    }
    TRACE("there is no IN event");
    return {PollHandleHintTag::NONE};
  }
  bool stop() { return true; }

  std::shared_ptr<HG> handler_generator;

  TcpConnManager<H> tcp_conn_manager;
};

TCP_NAMESPACE_END
#endif
