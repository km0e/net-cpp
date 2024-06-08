#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_SERVER_H_
#  define _XSL_NET_TRANSPORT_TCP_SERVER_H_
#  include "xsl/net/sync.h"
#  include "xsl/net/sync/poller.h"
#  include "xsl/net/transport/tcp/conn.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/utils.h"

#  include <spdlog/spdlog.h>
#  include <unistd.h>

#  include <memory>

TCP_NAMESPACE_BEGIN

template <class T, class H>
concept TcpHandlerGeneratorLike = TcpHandlerLike<H> && requires(T t, H h, int fd) {
  { t(fd) } -> std::same_as<std::unique_ptr<H>>;
};

template <TcpHandlerLike H, TcpHandlerGeneratorLike<H> HG>
class TcpServer {
public:
  TcpServer(TcpServer&&) = delete;
  TcpServer(std::shared_ptr<HG> handler_generator, TcpConnManagerConfig conn_manager_config)
      : handler_generator(handler_generator), tcp_conn_manager(conn_manager_config) {}
  ~TcpServer() {}
  PollHandleHint operator()(int fd, IOM_EVENTS events) {
    SPDLOG_TRACE("start accept");
    if ((events & IOM_EVENTS::IN) == IOM_EVENTS::IN) {
      sockaddr addr;
      socklen_t addr_len = sizeof(addr);
      int client_fd = ::accept(fd, &addr, &addr_len);
      if (client_fd == -1) {  // todo: handle error
        return {PollHandleHintTag::NONE};
      }
      if (!set_non_blocking(client_fd)) {
        SPDLOG_WARN("[TcpServer::<lambda::handler>] Failed to set non-blocking");
        close(client_fd);
        return {PollHandleHintTag::NONE};
      }
      auto handler = (*this->handler_generator)(client_fd);
      if (!handler) {
        SPDLOG_WARN("[TcpServer::<lambda::handler>] Failed to create handler");
        close(client_fd);
        return {PollHandleHintTag::NONE};
      }
      this->tcp_conn_manager.add(client_fd, std::move(handler));
      SPDLOG_TRACE("accept done");
      return {PollHandleHintTag::NONE};
    }
    SPDLOG_TRACE("there is no IN event");
    return {PollHandleHintTag::NONE};
  }
  bool stop() { return true; }

  std::shared_ptr<HG> handler_generator;

private:
  TcpConnManager<H> tcp_conn_manager;
};

TCP_NAMESPACE_END
#endif
