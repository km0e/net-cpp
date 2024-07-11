#pragma once

#include "xsl/net/transport/tcp/stream.h"
#ifndef _XSL_NET_HTTP_SERVER_H_
#  define _XSL_NET_HTTP_SERVER_H_
#  include "xsl/convert.h"
#  include "xsl/logctl.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/sync.h"
#  include "xsl/net/transport.h"

HTTP_NAMESPACE_BEGIN

template <Router R>
class Handler {
public:
  Handler(std::shared_ptr<R> router)
      : parser(), router(router), recv_task(), send_proxy(), keep_alive(false) {}
  Handler(Handler&&) = default;
  ~Handler() {}
  TcpHandleState recv(int fd) {
    auto res = recv_task.exec(fd);
    if (!res.has_value()) {
      if (res.error() == RecvErrorCategory::Eof) {
        return TcpHandleState::NONE;
      }
      // ERROR("recv error: {}", to_string_view(res.error()));
      return TcpHandleState::CLOSE;
    }
    size_t len = this->recv_task.data_buffer.size();
    // TODO: handle parse error
    auto req = this->parser.parse(this->recv_task.data_buffer.c_str(), len);
    if (!req.has_value()) {
      if (req.error().kind == ParseErrorKind::Partial) {
        return TcpHandleState::NONE;
      } else {
        // TODO: handle request error
        return TcpHandleState::CLOSE;
      }
    }
    RequestView view = req.value();
    if (auto it = view.headers.find("Content-Length"); it != view.headers.end()) {
      size_t content_len = std::strtoul(it->second.data(), nullptr, 10);
      len += content_len;
    }
    auto ctx = RouteContext{Request{this->recv_task.data_buffer.substr(0, len), req.value()}};
    recv_task.data_buffer.erase(0, len);
    auto rtres = this->router->route(ctx);
    if (rtres.is_err()) {
      // TODO: handle route error
      ERROR("route error: {}", xsl::to_string(rtres.error()));
      return TcpHandleState::CLOSE;
    }
    auto tasks = rtres.value()->into_send_tasks();
    tasks.splice_after(tasks.before_begin(), std::move(this->send_proxy.tasks));
    this->send_proxy.tasks = move(tasks);
    return this->send(fd);
  }
  TcpHandleState send(int fd) {
    auto res = send_proxy.exec(fd);
    if (!res.has_value()) {
      // ERROR("send error: {}", to_string_view(res.error()));
      return TcpHandleState::CLOSE;
    }
    if (!this->keep_alive) {
      return TcpHandleState::CLOSE;
    }
    return TcpHandleState::NONE;
  }
  void close([[maybe_unused]] int fd) { TRACE(""); }
  TcpHandleState other([[maybe_unused]] int fd, [[maybe_unused]] IOM_EVENTS events) {
    ERROR("Unexpected events");
    return TcpHandleState::CLOSE;
  }

private:
  HttpParser parser;
  std::shared_ptr<R> router;
  TcpRecvString<> recv_task;
  SendTasksProxy send_proxy;
  bool keep_alive;
};

using HttpHandler = Handler<HttpRouter>;

template <Router R, TcpHandlerLike H>
class HandlerGenerator {
public:
  HandlerGenerator(std::shared_ptr<R> router) : router(router) {}
  std::unique_ptr<Handler<R>> operator()([[maybe_unused]] int fd) {
    return make_unique<Handler<R>>(this->router);
  }

private:
  std::shared_ptr<R> router;
};

using HttpHandlerGenerator = HandlerGenerator<HttpRouter, HttpHandler>;

using HttpServer = TcpServer<Handler<HttpRouter>, HttpHandlerGenerator>;

HTTP_NAMESPACE_END
#endif
