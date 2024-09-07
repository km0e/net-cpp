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
  using aio_traits_type = AIOTraits<typename lower_type::io_dev_type>;
  using io_dev_type = typename aio_traits_type::value_type;
  using in_dev_type = typename aio_traits_type::in_dev_type;
  using out_dev_type = typename aio_traits_type::out_dev_type;
  using context_type = HandleContext<in_dev_type, out_dev_type>;
  using handler_type = Handler<in_dev_type, out_dev_type>;

  constexpr Server(lower_type&& server) : server(std::move(server)) {}

  constexpr Server(Server&&) = default;
  constexpr Server& operator=(Server&&) = default;
  constexpr ~Server() {}

  Task<void> serve_connection(auto&& service) {
    auto service_ptr = std::make_shared<std::remove_reference_t<decltype(service)>>(
        std::forward<decltype(service)>(service));
    typename lower_type::value_type conn;
    while (true) {
      auto [sz, err] = co_await this->server.read(std::span{&conn, 1});
      if (sz != 1 || err) {
        LOG2("accept error: {}", std::make_error_code(err.value()).message());
        continue;
      }
      http::serve_connection(std::move(conn), service_ptr).detach(co_await coro::GetExecutor());
    }
  }

private:
  lower_type server;
};

XSL_HTTP_NE
#endif
