#pragma once

#ifndef _XSL_NET_HTTP_SERVER_H_
#  define _XSL_NET_HTTP_SERVER_H_
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/sync.h"
#  include "xsl/net/transport.h"
#  include "xsl/wheel.h"

#  include <spdlog/spdlog.h>

HTTP_NAMESPACE_BEGIN

template <Router R>
class Handler {
public:
  Handler(shared_ptr<R> router)
      : parser(), router(router), recv_task(), send_proxy(), keep_alive(false) {}
  Handler(Handler&&) = default;
  ~Handler() {}
  TcpHandleState recv(int fd) {
    SPDLOG_TRACE("");
    auto res = recv_task.exec(fd);
    if (res.is_err()) {
      if (res.as_ref().unwrap_err() == RecvError::Eof) {
        return TcpHandleState::NONE;
      }
      SPDLOG_ERROR("recv error: {}", to_string(res.unwrap_err()));
      return TcpHandleState::CLOSE;
    }
    size_t len = this->recv_task.data_buffer.size();
    // TODO: handle parse error
    auto req = this->parser.parse(this->recv_task.data_buffer.c_str(), len);
    if (req.is_err()) {
      if (req.unwrap_err().kind == ParseErrorKind::Partial) {
        return TcpHandleState::NONE;
      } else {
        // TODO: handle request error
        return TcpHandleState::CLOSE;
      }
    }
    RequestView view = req.unwrap();
    if (auto it = view.headers.find("Content-Length"); it != view.headers.end()) {
      size_t content_len = std::strtoul(it->second.data(), nullptr, 10);
      len += content_len;
    }
    auto ctx = RouteContext{Request{this->recv_task.data_buffer.substr(0, len), req.unwrap()}};
    auto rtres = this->router->route(ctx);
    if (rtres.is_err()) {
      // TODO: handle route error
      SPDLOG_ERROR("route error: {}", wheel::to_string(rtres.unwrap_err()));
      return TcpHandleState::CLOSE;
    }
    auto tasks = rtres.unwrap()->into_send_tasks();
    tasks.splice_after(tasks.before_begin(), xsl::move(this->send_proxy.tasks));
    this->send_proxy.tasks = move(tasks);
    return this->send(fd);
  }
  TcpHandleState send(int fd) {
    SPDLOG_TRACE("");
    auto res = send_proxy.exec(fd);
    if (res.is_err()) {
      SPDLOG_ERROR("send error: {}", to_string(res.unwrap_err()));
      return TcpHandleState::CLOSE;
    }
    if (!this->keep_alive) {
      return TcpHandleState::CLOSE;
    }
    return TcpHandleState::NONE;
  }
  void close([[maybe_unused]] int fd) { SPDLOG_TRACE(""); }
  TcpHandleState other([[maybe_unused]] int fd, [[maybe_unused]] IOM_EVENTS events) {
    SPDLOG_ERROR("Unexpected events");
    return TcpHandleState::CLOSE;
  }

private:
  HttpParser parser;
  shared_ptr<R> router;
  TcpRecvString<> recv_task;
  SendTasksProxy send_proxy;
  bool keep_alive;
};

using HttpHandler = Handler<HttpRouter>;

template <Router R, TcpHandlerLike H>
class HandlerGenerator {
public:
  HandlerGenerator(shared_ptr<R> router) : router(router) {}
  unique_ptr<Handler<R>> operator()([[maybe_unused]] int fd) {
    return make_unique<Handler<R>>(this->router);
  }

private:
  shared_ptr<R> router;
};

using HttpHandlerGenerator = HandlerGenerator<HttpRouter, HttpHandler>;

using HttpServer = TcpServer<Handler<HttpRouter>, HttpHandlerGenerator>;
using HttpServerConfig = TcpServerConfig<Handler<HttpRouter>, HttpHandlerGenerator>;

HTTP_NAMESPACE_END
#endif
