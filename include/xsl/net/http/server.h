#pragma once

#ifndef _XSL_NET_HTTP_SERVER_H_
#  define _XSL_NET_HTTP_SERVER_H_
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/sync/poller.h"
#  include "xsl/wheel.h"

#  include <spdlog/spdlog.h>
HTTP_NAMESPACE_BEGIN

template <Router R>
class Handler {
public:
  Handler(shared_ptr<R> router) : parser{}, router{router}, recv_data{}, send_tasks{} {}
  Handler(Handler&&) = default;
  ~Handler() {}
  TcpHandleConfig init([[maybe_unused]]int fd) {
    SPDLOG_TRACE("");
    TcpHandleConfig cfg{};
    cfg.recv_tasks.emplace_front(make_unique<TcpRecvString>(this->recv_data));
    return cfg;
  }
  TcpHandleState recv([[maybe_unused]] TcpRecvTasks& tasks) {
    SPDLOG_TRACE("");
    if (this->recv_data.empty()) {
      return TcpHandleState{IOM_EVENTS::IN, TcpHandleHint::NONE};
    }
    size_t len = this->recv_data.size();
    auto req = this->parser.parse(recv_data.c_str(), len);
    if (req.is_err()) {
      if (req.unwrap_err().kind == ParseErrorKind::Partial) {
        return TcpHandleState{IOM_EVENTS::IN, TcpHandleHint::NONE};
      } else {
        // TODO: handle request error
        return TcpHandleState{IOM_EVENTS::NONE, TcpHandleHint::NONE};
      }
    }
    auto ctx = Context{Request{this->recv_data.substr(0, len), req.unwrap()}};
    auto rtres = this->router->route(ctx);
    if (rtres.is_err()) {
      // TODO: handle route error
      return TcpHandleState{IOM_EVENTS::NONE, TcpHandleHint::NONE};
    }
    this->recv_data.clear();
    this->send_tasks = move(rtres.unwrap()->into_send_tasks());
    return TcpHandleState{IOM_EVENTS::OUT, TcpHandleHint::WRITE};
  }
  TcpHandleState send(TcpSendTasks& tasks) {
    SPDLOG_TRACE("");
    if (this->send_tasks.empty()) {
      return TcpHandleState{IOM_EVENTS::NONE, TcpHandleHint::NONE};
    }
    tasks.splice_after(tasks.before_begin(), move(this->send_tasks));
    return TcpHandleState{IOM_EVENTS::OUT, TcpHandleHint::NONE};
  }

private:
  HttpParser parser;
  shared_ptr<R> router;
  string recv_data;
  TcpSendTasks send_tasks;
};

using HttpHandler = Handler<HttpRouter>;

template <Router R, TcpHandler H>
class HandlerGenerator {
public:
  HandlerGenerator(shared_ptr<R> router) : router(router) {}
  H operator()() { return Handler<R>{this->router}; }

private:
  shared_ptr<R> router;
};

using HttpHandlerGenerator = HandlerGenerator<HttpRouter, HttpHandler>;

using HttpServer = TcpServer<Handler<HttpRouter>, HttpHandlerGenerator>;
using HttpServerConfig = TcpServerConfig<Handler<HttpRouter>, HttpHandlerGenerator>;

HTTP_NAMESPACE_END
#endif
