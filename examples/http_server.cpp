#include <CLI/CLI.hpp>
#include <xsl/logctl.h>
#include <xsl/net.h>

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::net;
using namespace xsl::feature;
using namespace xsl::coro;
using namespace xsl;

Task<void> run(std::string_view ip, std::string_view port, std::shared_ptr<xsl::Poller> poller) {
  auto tcp_server = tcp::Server<Ip<4>>::create(poller, ip.data(), port.data());
  if (!tcp_server) {
    co_return;
  }
  using HttpServer = http::Server<std::remove_reference_t<decltype(*tcp_server)>>;
  auto http_router = std::make_shared<HttpServer::router_type>();
  auto get_handler = [](auto& hc) -> http::HandleResult {
    LOG4("{} {}", hc.request.view.method, hc.request.view.path);
    for (auto& [k, v] : hc.request.view.headers) {
      LOG4("{}: {}", k, v);
    }
    hc.easy_resp(http::Status::OK);
    co_return;
  };
  auto http_server = HttpServer(std::move(*tcp_server), http_router);
  http_server.add_route(http::Method::GET, "/", get_handler);
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
