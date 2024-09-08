/**
 * @file tcp_echo.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief A simple echo server
 * @version 0.11
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <CLI/CLI.hpp>
#include <xsl/coro.h>
#include <xsl/logctl.h>
#include <xsl/net.h>

#include <span>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::coro;
using namespace xsl;

Task<void> talk(std::string_view ip, std::string_view port, std::shared_ptr<xsl::Poller> poller) {
  using Server = tcp::Server<Ip<4>>;
  auto server = tcp::make_server<Ip<4>>(ip, port, poller).value();
  Server::value_type skt;
  while (true) {
    auto [sz, err] = co_await server.read(std::span<Server::value_type>(&skt, 1));
    if (err) {
      LOG3("accept error: {}", std::make_error_code(*err).message());
      break;
    }
    co_yield [](auto rw) mutable -> Task<void> {
      std::string buffer(4096, '\0');
      co_await net::splice(rw, rw, buffer);
    }(std::move(*skt));
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
  // talk<NewThreadExecutor>(ip, port, poller).detach(std::move(executor));
  talk(ip, port, poller).detach();
  while (true) {
    poller->poll();
  }
  return 0;
}
