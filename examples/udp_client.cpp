/**
 * @file udp_client.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.12
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <CLI/CLI.hpp>
#include <xsl/asio.h>
#include <xsl/coro.h>
#include <xsl/logctl.h>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::coro;
using namespace xsl::asio;
using namespace xsl::net;
using namespace xsl;

Task<void> talk(std::string_view ip, std::string_view port, std::shared_ptr<xsl::Poller> poller) {
  std::string buffer(4096, '\0');
  auto rw = AsyncSocket(gai_connect<UdpIpv4>(ip.data(), port.data()).value(), *poller);
  while (true) {
    std::cin >> buffer;
    auto [n, err] = co_await rw.write(std::as_bytes(std::span(buffer)));
    if (err.has_value()) {
      log_error("Failed to send data, err : {}", std::make_error_code(err.value()).message());
      break;
    }
    auto [n_recv, err_recv] = co_await rw.read(std::as_writable_bytes(std::span(buffer)));
    if (err_recv.has_value()) {
      log_error("Failed to recv data, err : {}", std::make_error_code(err_recv.value()).message());
      break;
    }
    log_info("Recv: {}", std::string_view{buffer.data(), n_recv});
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
  talk(ip, port, poller).detach(std::move(executor));
  // echo(ip, port, poller).detach();
  poller->run();
  return 0;
}
