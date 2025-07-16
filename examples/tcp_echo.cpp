/**
 * @file tcp_echo.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief A simple echo server
 * @version 0.3
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <CLI/CLI.hpp>
#include <xsl/asio.h>
#include <xsl/coro.h>
#include <xsl/io.h>
#include <xsl/logctl.h>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl;
using namespace xsl::asio;

Task<void> talk(std::string_view ip, std::string_view port, std::shared_ptr<Poller> poller) {
  auto util = make_socket_io_utils<Tcp<Ip<4>>>();
  auto creator = *util.make_creator(poller, ip, port);
  while (true) {
    auto task = co_await creator.accept().and_then(
        [&](auto &&skt) { return splice_bidirectional(skt, skt, *poller); });
    if (!task) {
      log_warning("splice error: {}", std::make_error_code(task.error()).message());
      break;
    }
    co_yield std::move(*task);
  }
  poller->shutdown();
  co_return;
}

int main(int argc, char *argv[]) {
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);
  log_info("Starting echo server at {}:{}", ip, port);

  auto poller = std::make_shared<xsl::Poller>();
  auto executor = std::make_shared<coro::NewThreadExecutor>();
  talk(ip, port, poller).detach(std::move(executor));
  poller->run();
  return 0;
}
