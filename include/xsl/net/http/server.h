#pragma once

#ifndef XSL_NET_HTTP_SERVER
#  define XSL_NET_HTTP_SERVER
#  include "xsl/coro/await.h"
#  include "xsl/coro/task.h"
#  include "xsl/logctl.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/http/stream.h"
#  include "xsl/net/transport/tcp/server.h"

#  include <string_view>

HTTP_NB

template <class Executor = coro::ExecutorBase>
coro::Task<void, Executor> http_connection(sys::io::AsyncReadWriteDevice dev,
                                           std::shared_ptr<HttpRouter> router) {
  auto [ard, awd] = std::move(dev).split();
  auto reader = HttpReader{std::move(ard)};
  while (true) {
    auto res = co_await reader.recv<Executor>();
    if (!res.has_value()) {
      LOG3("recv error: {}", std::make_error_code(res.error()).message());
      continue;
    }
    LOG5("{}", std::string_view(res->raw).substr(0, res->view.length));
    bool keep_alive = true;
    if (auto iter = res->view.headers.find("Connection");
        iter == res->view.headers.end() || iter->second != "keep-alive") {
      keep_alive = false;
    }
    auto ctx = RouteContext{std::move(*res)};
    auto rtres = router->route(ctx);
    while (true) {
      while (!rtres) {
        rtres = std::move(router->error_handle(rtres.error()));
      }
      auto handle_res = co_await (**rtres)(ctx);
      if (!handle_res) {
        LOG3("handle error: {}", to_string_view(handle_res.error()));
        continue;
      }
      auto [sz, err] = co_await handle_res->sendto(awd);
      if (err) {
        LOG3("send error: {}", std::make_error_code(err.value()).message());
      }
      break;
    }
    if (!keep_alive) {
      break;
    }
  }
}

class HttpServer {
public:
  HttpServer(transport::tcp::TcpServer server, std::shared_ptr<HttpRouter> router)
      : server(std::move(server)), router(std::move(router)) {}
  ~HttpServer() {}
  template <class Executor = coro::ExecutorBase>
  coro::Task<std::expected<void, std::errc>, Executor> run() {
    LOG4("HttpServer start");
    while (true) {
      auto res = co_await this->server.accept<Executor>();
      if (!res) {
        LOG2("accept error: {}", std::make_error_code(res.error()).message());
        continue;
      }
      auto [arwd, addr] = std::move(*res);
      http_connection<Executor>(std::move(arwd), this->router)
          .detach(co_await coro::GetExecutor<Executor>());
    }
  }

private:
  transport::tcp::TcpServer server;
  std::shared_ptr<HttpRouter> router;
};
HTTP_NE
#endif
