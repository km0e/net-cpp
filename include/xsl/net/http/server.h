#pragma once

#ifndef XSL_NET_HTTP_SERVER
#  define XSL_NET_HTTP_SERVER
#  include "xsl/coro/task.h"
#  include "xsl/logctl.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/http/stream.h"
#  include "xsl/net/transport/tcp/server.h"

#  include <string_view>

HTTP_NB

inline coro::Task<void> http_connection(sys::io::AsyncReadWriteDevice dev,
                                        std::shared_ptr<HttpRouter> router) {
  auto [ard, awd] = std::move(dev).split();
  auto reader = HttpReader{std::move(ard)};
  while (true) {
    auto res = co_await reader.recv();
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
    if (!rtres) {
      LOG2("route error: {}", xsl::to_string(rtres.error()));
      continue;
    }
    auto send_res = co_await (**rtres)(awd);
    if (!send_res) {
      LOG3("send error: {}", send_res.error().message());
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
  coro::Task<std::expected<void, std::errc>> run() {
    LOG4("HttpServer start");
    while (true) {
      auto res = co_await this->server.accept();
      if (!res) {
        LOG2("accept error: {}", std::make_error_code(res.error()).message());
        continue;
      }
      auto [arwd, addr] = std::move(*res);
      http_connection(std::move(arwd), this->router).detach();
    }
  }

private:
  transport::tcp::TcpServer server;
  std::shared_ptr<HttpRouter> router;
};
HTTP_NE
#endif
