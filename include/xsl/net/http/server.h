#pragma once

#ifndef _XSL_NET_HTTP_SERVER_H_
#  define _XSL_NET_HTTP_SERVER_H_
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/sync.h"
#  include "xsl/wheel.h"

#  include <spdlog/spdlog.h>
HTTP_NAMESPACE_BEGIN

template <Router R>
class Handler {
public:
  Handler(shared_ptr<R> router)
      : parser(), router(router), recv_task(), send_proxy(), keep_alive(true) {}
  Handler(Handler&&) = default;
  ~Handler() {}
  PollHandleHint recv(int fd) {
    SPDLOG_TRACE("");
    auto res = recv_task.exec(fd);
    if (res.is_err()) {
      if (res.as_ref().unwrap_err() == RecvError::Eof) {
        return {PollHandleHintTag::DELETE};
      }
      SPDLOG_ERROR("recv error: {}", to_string(res.unwrap_err()));
      return {PollHandleHintTag::DELETE};
    }
    size_t len = this->recv_task.data_buffer.size();
    // TODO: handle parse error
    auto req = this->parser.parse(this->recv_task.data_buffer.c_str(), len);
    if (req.is_err()) {
      if (req.unwrap_err().kind == ParseErrorKind::Partial) {
        return {PollHandleHintTag::NONE};
      } else {
        // TODO: handle request error
        return {PollHandleHintTag::NONE};
      }
    }
    auto ctx = Context{Request{this->recv_task.data_buffer.substr(0, len), req.unwrap()}};
    auto rtres = this->router->route(ctx);
    if (rtres.is_err()) {
      // TODO: handle route error
      SPDLOG_ERROR("route error: {}", wheel::to_string(rtres.unwrap_err()));
      return {PollHandleHintTag::DELETE};
    }
    auto tasks = rtres.unwrap()->into_send_tasks();
    tasks.splice_after(tasks.before_begin(), xsl::move(this->send_proxy.tasks));
    this->send_proxy.tasks = move(tasks);
    this->send(fd);
    return {PollHandleHintTag::NONE};
  }
  PollHandleHint send(int fd) {
    SPDLOG_TRACE("");
    auto res = send_proxy.exec(fd);
    if (res.is_err()) {
      SPDLOG_ERROR("send error: {}", to_string(res.unwrap_err()));
      return {PollHandleHintTag::DELETE};
    }
    return {PollHandleHintTag::NONE};
  }
  PollHandleHint other([[maybe_unused]] int fd, [[maybe_unused]] IOM_EVENTS events) {
    SPDLOG_ERROR("Unexpected events");
    return {PollHandleHintTag::NONE};
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
