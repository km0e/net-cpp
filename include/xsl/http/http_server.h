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

class HandlerGenerator : public transport::HandlerGenerator {
public:
  HandlerGenerator(wheel::shared_ptr<HttpRouter> router);
  transport::Handler operator()() override;

private:
  wheel::shared_ptr<HttpRouter> router;
};

class HttpHandler {
public:
  HttpHandler(wheel::shared_ptr<HttpRouter> router);
  ~HttpHandler();
  transport::HandleState operator()(int read_write, wheel::string& data);

private:
  HttpParser parser;
  wheel::shared_ptr<HttpRouter> router;
};
HTTP_NAMESPACE_END
#endif
