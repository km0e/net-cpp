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
XSL_ASIO_NB
template <class LowerCreator>
class HttpServer {
public:
  using lower_type = LowerCreator;

  template <class T>
  struct extract_io_dev_type {
    using type = T;
  };
  template <class T>
    requires requires { typename T::io_dev_type; }
  struct extract_io_dev_type<T> {
    using type = typename T::io_dev_type;
  };

  using io_dev_type = typename extract_io_dev_type<lower_type>::type;

  using context_type = HandleContext<io_dev_type, io_dev_type>;
  using handler_type = Handler<io_dev_type, io_dev_type>;

  constexpr HttpServer(lower_type&& server) noexcept(
      std::is_nothrow_move_constructible_v<lower_type>)
      : server(std::move(server)) {}

  constexpr HttpServer(HttpServer&&) = default;
  constexpr HttpServer& operator=(HttpServer&&) = default;
  constexpr ~HttpServer() {}

  Task<void> serve_connection(auto&& service) {
    auto service_ptr = std::make_shared<std::remove_reference_t<decltype(service)>>(
        std::forward<decltype(service)>(service));
    auto& io = co_await CurrentIOContext;
    while (true) {
      auto res = co_await this->server->accept_async(io);
      if (!res) {
        if (io.stopped()
            || (res.error() != errc::operation_would_block
                && res.error() != errc::resource_unavailable_try_again
                && res.error() != errc::connection_aborted)) {
          // poller shut down or a persistent accept error, stop serving
          log_info("accept stopped: {}", std::make_error_code(res.error()).message());
          co_return;
        }
        continue;
      }
      co_yield asio::serve_connection(std::move(*res), service_ptr);
    }
  }

private:
  lower_type server;
};
XSL_ASIO_NE
#endif
