#pragma once

#ifndef _XSL_NET_HTTP_SERVER_H_
#  define _XSL_NET_HTTP_SERVER_H_
#  include <spdlog/spdlog.h>
#  include <xsl/http/http.h>
#  include <xsl/http/msg.h>
#  include <xsl/http/parse.h>
#  include <xsl/http/router.h>
#  include <xsl/sync/poller.h>
#  include <xsl/transport/tcp/server.h>
#  include <xsl/utils/wheel/wheel.h>

#  include "xsl/http/context.h"
#  include "xsl/transport/tcp/context.h"
#  include "xsl/transport/tcp/helper.h"
HTTP_NAMESPACE_BEGIN

template <Router R>
class Handler {
private:
  using TcpHandleState = transport::tcp::HandleState;
  using TcpHandleHint = transport::tcp::HandleHint;

public:
  Handler(wheel::shared_ptr<R> router) : router(router) {}
  ~Handler() {}
  transport::tcp::HandleConfig init() {
    transport::tcp::HandleConfig cfg{};
    cfg.recv_tasks.emplace_front(transport::tcp::RecvString::create(this->recv_data));
    return cfg;
  }
  TcpHandleState recv([[maybe_unused]] transport::tcp::RecvTasks& tasks) {
    spdlog::trace("[http::Handler::recv]");
    if (this->recv_data.empty()) {
      return TcpHandleState{sync::IOM_EVENTS::IN, TcpHandleHint::NONE};
    }
    size_t len = this->recv_data.size();
    auto req = this->parser.parse(recv_data.c_str(), len);
    if (req.is_err()) {
      if (req.unwrap_err().kind == ParseErrorKind::Partial) {
        return TcpHandleState{sync::IOM_EVENTS::IN, TcpHandleHint::NONE};
      } else {
        // TODO: handle request error
        return TcpHandleState{sync::IOM_EVENTS::NONE, TcpHandleHint::NONE};
      }
    }
    auto ctx = Context{Request{this->recv_data.substr(0, len), req.unwrap()}};
    auto rtres = this->router->route(ctx);
    if (rtres.is_err()) {
      // TODO: handle route error
      return TcpHandleState{sync::IOM_EVENTS::NONE, TcpHandleHint::NONE};
    }
    this->recv_data.clear();
    this->send_data = wheel::move(rtres.unwrap().to_string());
    return TcpHandleState{sync::IOM_EVENTS::OUT, TcpHandleHint::WRITE};
  }
  TcpHandleState send(transport::tcp::SendTasks& tasks) {
    tasks.emplace_front(transport::tcp::SendString::create(wheel::move(this->send_data)));
    return TcpHandleState{sync::IOM_EVENTS::NONE, TcpHandleHint::NONE};
  }

private:
  HttpParser parser;
  wheel::shared_ptr<R> router;
  wheel::string send_data;
  wheel::string recv_data;
};

using DefaultHandler = Handler<DefaultRouter>;

template <Router R, transport::tcp::Handler H>
class HandlerGenerator {
public:
  HandlerGenerator(wheel::shared_ptr<R> router) : router(router) {}
  H operator()() { return Handler<R>{this->router}; }

private:
  wheel::shared_ptr<R> router;
};

using DefaultHG = HandlerGenerator<DefaultRouter, DefaultHandler>;

using DefaultServer = transport::tcp::TcpServer<Handler<DefaultRouter>, DefaultHG>;

HTTP_NAMESPACE_END
#endif
