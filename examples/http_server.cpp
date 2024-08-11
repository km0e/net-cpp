#include <CLI/CLI.hpp>
#include <xsl/logctl.h>
#include <xsl/net.h>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::net;
using namespace xsl::feature;
using namespace xsl::coro;
using namespace xsl;

Task<void> run(std::string_view ip, std::string_view port, std::shared_ptr<xsl::Poller> poller) {
  auto server = http::Server<Tcp<Ip<4>>>::create(ip, port, poller).value();
  server.redirect(http::Method::GET, "/", "/index.html");
  server.add_static("/", "./build/html/");
  co_await server.run();
  poller->shutdown();
  co_return;
}

int main(int argc, char* argv[]) {
  set_log_level(xsl::LogLevel::LOG4);
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);

  auto poller = std::make_shared<xsl::Poller>();
  run(ip, port, poller).detach();
  while (poller->valid()) {
    poller->poll();
  }
  return 0;
}
