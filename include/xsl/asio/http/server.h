/**
 * @file server.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP server
 * @version 0.1.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_HTTP_SERVER
#  define XSL_ASIO_HTTP_SERVER
#  include <xsl/asio/http/conn.h>
#  include <xsl/asio/http/context.h>
#  include <xsl/asio/http/def.h>
#  include <xsl/coro.h>
#  include <xsl/log.h>

#  include <memory>
#  include <utility>

XSL_ASIO_HTTP_NB
template <class LowerCreator>
class Server {
public:
  using lower_type = LowerCreator;

  using io_dev_type = typename lower_type::io_dev_type;
  using context_type = HandleContext<io_dev_type, io_dev_type>;
  using handler_type = Handler<io_dev_type, io_dev_type>;

  constexpr Server(lower_type&& server) : server(std::move(server)) {}

  constexpr Server(Server&&) = default;
  constexpr Server& operator=(Server&&) = default;
  constexpr ~Server() {}

  Task<void> serve_connection(auto&& service) {
    auto service_ptr = std::make_shared<std::remove_reference_t<decltype(service)>>(
        std::forward<decltype(service)>(service));
    while (true) {
      auto res = co_await this->server.accept();
      if (!res) {
        log_error("accept error: {}", std::make_error_code(res.error()).message());
        continue;
      }
      co_yield http::serve_connection(std::move(*res), service_ptr);
    }
  }

private:
  lower_type server;
};

XSL_ASIO_HTTP_NE
#endif
