/**
 * @file udp_client.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief UDP echo client example
 * @version 0.2.0
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <CLI/CLI.hpp>
#include <xsl/asio.h>
#include <xsl/coro.h>
#include <xsl/log.h>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::asio;
using namespace xsl;

Task<void> talk(std::string_view ip, std::string_view port, std::shared_ptr<xsl::Context> ctx) {
  byte buffer[4096]{};
  auto util = make_async_socket_utils<UdpIpv4>();
  auto rw = *util.c2(*ctx, ip.data(), port.data());
  while (true) {
    std::cin.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
    auto res = co_await rw.write(buffer, std::cin.gcount());
    if (!res) {
      log_error("Failed to send data, err : {}", res.message());
      break;
    }
    res = co_await rw.read(buffer, 4096);
    if (!res) {
      log_error("Failed to recv data, err : {}", res.message());
      break;
    }
    std::println(std::cout, "{}\n",
                 std::string_view(reinterpret_cast<const char *>(buffer), res.size));
  }
  ctx->shutdown();
  co_return;
}

int main(int argc, char *argv[]) {
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);

  auto poller = std::make_shared<xsl::Context>();
  auto executor = std::make_shared<coro::NewThreadExecutor>();
  talk(ip, port, poller).detach(std::move(executor));
  poller->run();
  return 0;
}
