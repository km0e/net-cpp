#include <CLI/CLI.hpp>
#include <xsl/logctl.h>
#include <xsl/net.h>

#include <expected>
#include <memory>
#include <string>
#include <string_view>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::net;
using namespace xsl::feature;
using namespace xsl::coro;
using namespace xsl;

Task<void> run(std::string_view ip, std::string_view port, std::shared_ptr<xsl::Poller> poller) {
  using HttpServer = http::Server<Tcp<Ip<4>>>;
  auto server = HttpServer::create(ip.data(), port.data(), poller);
  if (!server) {
    co_return;
  }
  using context_type = typename HttpServer::context_type;
  auto get_handler = [](context_type& hc) -> http::HandleResult {
    LOG4("{} {}", hc.request.view.method, hc.request.view.path);
    for (auto& [k, v] : hc.request.view.headers) {
      LOG4("{}: {}", k, v);
    }
    hc.easy_resp(http::Status::OK);
    co_return;
  };
  server->add_route(http::Method::GET, "/", get_handler);
  co_await server->run();
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
