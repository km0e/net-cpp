#include "xsl/logctl.h"

#include <CLI/CLI.hpp>
#include <xsl/net.h>

#include <memory>
#include <string>
#include <utility>

std::string ip = "127.0.0.1";
std::string port = "8080";
using namespace xsl::net;
using namespace xsl::feature;

xsl::coro::Task<void> session(TcpStream stream) {
  for (std::string buf;; buf.clear()) {
    if (!co_await stream.read(buf)) break;
    if (!co_await stream.write(buf)) break;
  }
}

xsl::coro::Task<void> echo(xsl::Socket &&socket, std::shared_ptr<xsl::Poller> poller) {
  auto acceptor = Acceptor{std::move(socket), poller};
  for (;;) {
    auto ares = co_await acceptor;
    if (!ares) {
      WARNING("acceptor error: {}", ares.error().message());
    }
    auto [skt, ni] = std::move(ares.value());
    session(TcpStream{std::move(skt), poller}).detach();
  }
}

int main(int argc, char *argv[]) {
  xsl::no_log();
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);

  auto server = transport::serv<Ip<4>, Tcp>(ip.c_str(), port.c_str());
  if (!server) {
    return 1;
  }
  auto poller = std::make_shared<xsl::Poller>();
  echo(std::move(server.value()), poller).detach();
  while (true) {
    poller->poll();
  }
  return 0;
}
