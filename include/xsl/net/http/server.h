/**
 * @file server.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP server
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_HTTP_SERVER
#  define XSL_NET_HTTP_SERVER
#  include "xsl/ai.h"
#  include "xsl/coro.h"
#  include "xsl/logctl.h"
#  include "xsl/net/http/conn.h"
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/def.h"

#  include <expected>
#  include <memory>
#  include <utility>

XSL_HTTP_NB
template <class LowerServer>
class Server {
public:
  using lower_type = LowerServer;
  using io_dev_type = typename lower_type::io_dev_type;
  using abr_type = typename lower_type::in_dev_type;
  using abw_type = typename lower_type::out_dev_type;
  using context_type = HandleContext<abr_type, abw_type>;
  using handler_type = Handler<abr_type, abw_type>;

  Server(lower_type&& server) : server(std::move(server)) {}

  Server(Server&&) = default;
  Server& operator=(Server&&) = default;
  ~Server() {}

  template <class Executor = coro::ExecutorBase, class Service>
  Task<void, Executor> serve_connection(Service&& service) {
    auto service_ptr = std::make_shared<Service>(std::forward<Service>(service));
    typename lower_type::value_type conn;
    while (true) {
      auto [sz, err] = co_await ai::read_poly_resolve<Executor>(this->server, std::span{&conn, 1});
      if (sz != 1 || err) {
        LOG2("accept error: {}", std::make_error_code(err.value()).message());
        continue;
      }
      http::serve_connection<Executor>(std::move(conn), service_ptr)
          .detach(co_await coro::GetExecutor<Executor>());
    }
  }

private:
  lower_type server;
};

XSL_HTTP_NE
#endif
