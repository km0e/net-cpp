#pragma once

#ifndef _XSL_NET_HTTP_SERVER_H_
#  define _XSL_NET_HTTP_SERVER_H_
#  include <spdlog/spdlog.h>
#  include <xsl/http/http.h>
#  include <xsl/http/msg.h>
#  include <xsl/http/router.h>
#  include <xsl/sync/poller.h>
#  include <xsl/transport/tcp/server.h>
#  include <xsl/utils/wheel/wheel.h>

#  include "xsl/http/context.h"
#  include "xsl/transport/tcp/context.h"
#  include "xsl/transport/tcp/helper.h"
HTTP_NAMESPACE_BEGIN
class HttpParser {
public:
  HttpParser();
  ~HttpParser();
  wheel::vector<RequestResult> parse(const char* data, size_t len);
};
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
    auto reqs = this->parser.parse(recv_data.c_str(), recv_data.size());
    wheel::string res;
    res.reserve(1024);
    for (auto& req : reqs) {
      if (req.is_err()) {
        res += "HTTP/1.1 400 Bad Request\r\n\r\n";
        continue;
      }
      auto ctx = Context{req.unwrap()};
      auto tmpres = this->router->route(ctx);
      if (tmpres.is_err()) {
        res += "HTTP/1.1 404 Not Found\r\n\r\n";
        continue;
      }
      res += tmpres.unwrap().to_string();
    }
    this->recv_data.clear();
    this->send_data = wheel::move(res);
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
