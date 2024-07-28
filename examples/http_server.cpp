#include "xsl/logctl.h"
#include "xsl/net/http/msg.h"
#include "xsl/net/http/proto.h"
#include "xsl/net/http/router.h"
#include "xsl/net/http/server.h"

#include <CLI/CLI.hpp>
#include <xsl/net.h>
#include <xsl/sys/pipe.h>

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::net;
using namespace xsl::feature;
using namespace xsl::coro;
using namespace xsl;

Task<void> run(std::string_view ip, std::string_view port, std::shared_ptr<xsl::Poller> poller) {
  auto tcp_server = tcp::Server::create<Ip<4>, Tcp>(poller, ip.data(), port.data());
  if (!tcp_server) {
    co_return;
  }
  auto http_router = std::make_shared<http::HttpRouter>();
  auto get_handler = [](auto&) -> http::RouteHandleResult {
    co_return http::HttpResponse{{HttpVersion::HTTP_1_1, 200, "OK"}};
  };
  if (!http_router->add_route(http::HttpMethod::GET, "/", get_handler)) {
    LOG5("add_route failed");
    co_return;
  }
  auto http_server = http::HttpServer(std::move(*tcp_server), http_router);
  co_await http_server.run();
}

int main(int argc, char* argv[]) {
  set_log_level(xsl::LogLevel::LOG5);
  // xsl::no_log();
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);

  auto poller = std::make_shared<xsl::Poller>();
  run(ip, port, poller).detach();
  while (true) {
    poller->poll();
  }
  return 0;
}
