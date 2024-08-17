#include <CLI/CLI.hpp>
#include <xsl/coro.h>
#include <xsl/logctl.h>
#include <xsl/net.h>

#include <span>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::feature;
using namespace xsl::coro;
using namespace xsl;

template <class Executor = ExecutorBase>
Lazy<void, Executor> echo(std::string_view ip, std::string_view port,
                          std::shared_ptr<xsl::Poller> poller) {
  using Server = tcp::Server<Ip<4>>;
  auto server = Server::create(ip, port, poller).value();
  Server::value_type skt;
  while (true) {
    auto [sz, err] = co_await server.read<Executor>(std::span<Server::value_type>(&skt, 1));
    if (err) {
      LOG3("accept error: {}", std::make_error_code(*err).message());
      break;
    }
    auto [r, w] = std::move(*skt).split();
    [](auto r, auto w) mutable -> Lazy<void, Executor> {
      std::string buffer(4096, '\0');
      co_await net::splice<Executor>(r, w, buffer);
    }(std::move(r), std::move(w))
                                      .detach(co_await coro::GetExecutor<Executor>());
  }
  poller->shutdown();
  co_return;
}

int main(int argc, char *argv[]) {
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);

  auto poller = std::make_shared<xsl::Poller>();
  auto executor = std::make_shared<NewThreadExecutor>();
  echo<NewThreadExecutor>(ip, port, poller).detach(std::move(executor));
  // echo(ip, port, poller).detach();
  while (true) {
    poller->poll();
  }
  return 0;
}
