#include "xsl/feature.h"
#include "xsl/logctl.h"
#include "xsl/net/transport/tcp.h"
#include "xsl/net/transport/tcp/stream.h"
#include "xsl/sys.h"

#include <CLI/CLI.hpp>
#include <xsl/net.h>
#include <xsl/net/transport.h>

#include <memory>
#include <string>
#include <thread>
#include <utility>

std::string ip = "127.0.0.1";
std::string port = "8080";
using namespace xsl::net;
using namespace xsl::feature;

xsl::coro::Task<void> session(TcpStream stream) {
  std::string buf;
  for (;;) {
    buf.clear();
    auto rres = co_await stream.read(buf);
    if (!rres) {
      if (rres.error().eof()) {
        INFO("connection closed");
        break;
      } else {
        WARNING("read error: {}", rres.error().message());
        break;
      }
    }
    INFO("Received: {}", buf);
    auto sres = co_await stream.write(buf);
    if (!sres) {
      WARNING("write error: {}", sres.error().message());
      break;
    }
    INFO("Sent: {}", buf);
  }
  co_return;
}

xsl::coro::Task<void> echo(xsl::Socket &&socket) {
  auto poller = std::make_shared<xsl::Poller>();
  std::thread{[poller] {
    while (true) {
      poller->poll();
    }
  }}.detach();
  auto acceptor = Acceptor{std::move(socket), poller};
  for (;;) {
    auto ares = co_await acceptor;
    if (!ares) {
      WARNING("acceptor error: {}", ares.error().message());
    }
    auto [skt, ni] = std::move(ares.value());
    INFO("Accepted: {}:{}", ni.ip, ni.port);
    session(TcpStream{std::move(skt), poller}).detach();
    INFO("Accepted");
  }
}

int main(int argc, char *argv[]) {
  xsl::set_log_level(xsl::LogLevel::TRACE);
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);
  INFO("Echo server: {}:{}", ip, port);
  auto server = transport::serv<Ip<4>, Tcp>(ip.c_str(), port.c_str());
  if (!server) {
    return 1;
  }
  echo(std::move(server.value())).block();
  return 0;
}
