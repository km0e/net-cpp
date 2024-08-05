#include <CLI/CLI.hpp>
#include <xsl/logctl.h>
#include <xsl/net.h>

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
  auto get_handler = [](auto& rc) -> http::RouteHandleResult {
    LOG4("{} {}", rc.request.view.method, rc.request.view.path);
    for (auto& [k, v] : rc.request.view.headers) {
      LOG4("{}: {}", k, v);
    }
    co_return http::HttpResponse{{http::HttpVersion::HTTP_1_1, http::HttpStatus::OK}};
  };
  http_router->add_route(http::HttpMethod::GET, "/", get_handler);
  auto http_server = http::HttpServer(std::move(*tcp_server), http_router);
  co_await http_server.run();
}

int main(int argc, char* argv[]) {
  set_log_level(xsl::LogLevel::LOG4);
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
