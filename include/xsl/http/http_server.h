#pragma once
#ifndef _XSL_NET_HTTP_SERVER_H_
#  define _XSL_NET_HTTP_SERVER_H_
#  include <xsl/config.h>
#  include <xsl/http/config.h>
#  include <xsl/http/http_msg.h>
#  include <xsl/http/http_router.h>
#  include <xsl/sync/poller.h>
#  include <xsl/transport/tcp_server.h>
#  include <xsl/utils/wheel/wheel.h>
HTTP_NAMESPACE_BEGIN
class HttpParser {
public:
  HttpParser();
  ~HttpParser();
  wheel::vector<RequestResult> parse(const char* data, size_t len);
};
template <Router R> class HttpHandler {
public:
  HttpHandler(wheel::shared_ptr<R> router) : router(router) {}
  ~HttpHandler() {}
  transport::HandleState operator()(int read_write, wheel::string& data) {
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

template <Router R> class HandlerGenerator : public transport::HandlerGenerator {
public:
  HandlerGenerator(wheel::shared_ptr<R> router) : router(router) {}
  transport::Handler operator()() { return HttpHandler<R>{this->router}; }

private:
  wheel::shared_ptr<R> router;
};

using DefaultHandlerGenerator = HandlerGenerator<HttpRouter>;

HTTP_NAMESPACE_END
#endif
