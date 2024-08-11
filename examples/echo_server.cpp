#include <CLI/CLI.hpp>
#include <xsl/coro.h>
#include <xsl/logctl.h>
#include <xsl/net.h>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::feature;
using namespace xsl::coro;
using namespace xsl;

Task<void> echo(std::string_view ip, std::string_view port, std::shared_ptr<xsl::Poller> poller) {
  auto server = tcp::Server<Ip<4>>::create(ip, port, poller).value();
  while (true) {
    auto ac_res = co_await server.accept(nullptr);
    if (!ac_res) {
      continue;
    }
    auto [r, w] = std::move(*ac_res).split();
    [](auto r, auto w) mutable -> Task<void> {
      std::string buffer(4096, '\0');
      co_await net::splice(r, w, buffer);
    }(std::move(r), std::move(w))
                                      .detach(co_await coro::GetExecutor());
  }
  poller->shutdown();
  co_return;
}

int main(int argc, char *argv[]) {
  set_log_level(xsl::LogLevel::LOG4);
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
