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
#  include <type_traits>
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
  Lazy<void, Executor> serve_connection(Service&& service) {
    auto service_ptr = std::make_shared<Service>(std::forward<Service>(service));
    typename lower_type::value_type conn;
    while (true) {
      auto [sz, err] = co_await [&]() {
        if constexpr (std::is_same_v<ABR, typename lower_type::in_dev_type>) {
          return this->server.read(std::span{&conn, 1});
        } else {
          return this->server.template read<Executor>(std::span{&conn, 1});
        }
      }();
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
