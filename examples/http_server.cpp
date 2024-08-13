
#include <CLI/CLI.hpp>
#include <xsl/coro.h>
#include <xsl/logctl.h>
#include <xsl/net.h>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::net;
using namespace xsl::feature;
using namespace xsl::coro;
using namespace xsl;

template <class Executor = ExecutorBase>
Lazy<void, Executor> run(std::string_view ip, std::string_view port,
                         std::shared_ptr<xsl::Poller> poller) {
  auto server_builder = http::ServerBuilder<Tcp<Ip<4>>>{};
  server_builder.redirect(http::Method::GET, "/", "/index.html");
  server_builder.add_static("/", {"./build/html/", {"br"}});
  auto server = std::move(server_builder).build(ip, port, poller).value();
  co_await server.template run<Executor>();
  poller->shutdown();
  co_return;
}

int main(int argc, char* argv[]) {
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);

  auto poller = std::make_shared<xsl::Poller>();
  auto executor = std::make_shared<NewThreadExecutor>();
  // run<NewThreadExecutor>(ip, port, poller).detach(std::move(executor));
  run(ip, port, poller).detach();
  while (poller->valid()) {
    poller->poll();
  }
  return 0;
}
