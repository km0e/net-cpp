#include "xsl/logctl.h"

#include <CLI/CLI.hpp>
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

xsl::coro::Task<void> session(TcpStream stream) {
  for (std::string buf;; buf.clear()) {
    if (!co_await stream.unsafe_read(tcp::StringWriter(buf))) break;
    if (!co_await stream.unsafe_write(tcp::StringReader(buf))) break;
  }
}

Task<void> echo(std::string_view ip, std::string_view port, std::shared_ptr<xsl::Poller> poller) {
  auto server = tcp::Server::create<Ip<4>, Tcp>(poller, ip.data(), port.data());
  if (!server) {
    co_return;
  }
  auto serv = std::move(server.value());
  while (true) {
    auto _
        = (co_await serv.accept()).and_then([&](auto &&stream) -> std::expected<void, std::errc> {
            //  session(std::move(stream)).detach();
            auto fwdres = tcp::forward(poller, stream, stream);
            if (!fwdres) return std::unexpected{fwdres.error()};
            auto [fwd, lazy_read, lazy_write] = std::move(*fwdres);
            lazy_read.detach();
            lazy_write.detach();
            return std::expected<void, std::errc>{};
          });
  }
}

int main(int argc, char *argv[]) {
  xsl::set_log_level(xsl::LogLevel::DEBUG);
  // xsl::no_log();
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
