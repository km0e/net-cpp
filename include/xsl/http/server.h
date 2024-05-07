#pragma once
#ifndef _XSL_NET_HTTP_SERVER_H_
#  define _XSL_NET_HTTP_SERVER_H_
#  include <xsl/http/http.h>
#  include <xsl/http/msg.h>
#  include <xsl/http/router.h>
#  include <xsl/sync/poller.h>
#  include <xsl/transport/server.h>
#  include <xsl/utils/wheel/wheel.h>
HTTP_NAMESPACE_BEGIN
class HttpParser {
public:
  HttpParser();
  ~HttpParser();
  wheel::vector<RequestResult> parse(const char* data, size_t len);
};
template <Router R>
class Handler {
public:
  Handler(wheel::shared_ptr<R> router) : router(router) {}
  ~Handler() {}
  transport::HandleState handle(int read_write, wheel::string& data) {
    if (read_write == 0) {
      auto reqs = this->parser.parse(data.c_str(), data.size());
      wheel::string res;
      res.reserve(1024);
      for (auto& req : reqs) {
        if (req.is_err()) {
          res += "HTTP/1.1 400 Bad Request\r\n\r\n";
          continue;
        }
        auto tmp = req.unwrap();
        auto tmpres = this->router->route(tmp);
        if (tmpres.is_err()) {
          res += "HTTP/1.1 404 Not Found\r\n\r\n";
          continue;
        }
        res += (*tmpres.unwrap())(std::move(tmp)).to_string();
      }
      data = res;
      return transport::HandleState{sync::IOM_EVENTS::OUT, transport::HandleHint::WRITE};
    } else {
      return transport::HandleState{sync::IOM_EVENTS::NONE, transport::HandleHint::NONE};
    }
  }

private:
  HttpParser parser;
  wheel::shared_ptr<R> router;
};

using DefaultHandler = Handler<DefaultRouter>;

template <Router R, transport::Handler H>
class DefaultHandlerGenerator {
public:
  DefaultHandlerGenerator(wheel::shared_ptr<R> router) : router(router) {}
  H generate() { return Handler<R>{this->router}; }

private:
  wheel::shared_ptr<R> router;
};

using DefaultHG = DefaultHandlerGenerator<DefaultRouter, DefaultHandler>;

using DefaultServer = transport::TcpServer<Handler<DefaultRouter>, DefaultHG>;

HTTP_NAMESPACE_END
#endif
