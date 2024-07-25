#include "xsl/io/splice.h"
#include "xsl/logctl.h"

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

Task<void> echo(std::string_view ip, std::string_view port, std::shared_ptr<xsl::Poller> poller) {
  auto server = tcp::Server::create<Ip<4>, Tcp>(poller, ip.data(), port.data());
  if (!server) {
    co_return;
  }
  auto serv = std::move(server.value());
  while (true) {
    auto ac_res = co_await serv.accept();
    if (!ac_res) {
      continue;
    }
    auto [rw, addr] = std::move(*ac_res);
    auto [r, w] = std::move(rw).split();
    io::splice(std::move(r), std::move(w), std::string(4096, '\0')).detach();
  }
}

int main(int argc, char *argv[]) {
  // set_log_level(xsl::LogLevel::LOG1);
  xsl::no_log();
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);

  auto poller = std::make_shared<xsl::Poller>();
  echo(ip, port, poller).detach();
  while (true) {
    poller->poll();
  }
  return 0;
}
