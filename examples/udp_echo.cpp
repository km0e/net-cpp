/**
 * @file udp_echo.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief UDP echo server
 * @version 0.2
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <CLI/CLI.hpp>
#include <xsl/asio.h>
#include <xsl/coro.h>
#include <xsl/feature.h>
#include <xsl/log.h>
#include <xsl/sys.h>
#include <xsl/wheel.h>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::asio;
using namespace xsl;

Task<void> talk(std::string_view ip, std::string_view port) {
  byte buffer[4096]{};
  auto util = AsyncSocketCreatorCompose<UdpIpv4>();
  auto& ctx = co_await CurrentIOContext;
  auto rw = *util.c(ctx, ip.data(), port.data());
  auto addr = sys::net::make_sockaddr<UdpIpv4>();
  std::string dst(128, '\0');
  std::uint16_t port_num;
  while (true) {
    auto res = co_await rw->recvfrom(addr, buffer, 4096);
    if (!res) {
      log_debug("Error: {}", res.message());
      break;
    }
    if (auto res = addr.parse(dst, port_num); res != errc{}) {
      log_debug("Error: {}", to_string_view(res));
      continue;
    }
    std::println(std::cout, "<{},{}>: {}", dst, port_num,
                 std::string_view(reinterpret_cast<const char*>(buffer), res.size));
    res = co_await rw->sendto(addr, buffer, res.size);
    if (!res) {
      log_debug("Error: {}", res.message());
      break;
    }
    log_info("Sent: {}", std::string_view(reinterpret_cast<const char*>(buffer), res.size));
  }
  ctx.shutdown();
  co_return;
}

int main(int argc, char* argv[]) {
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);

  MUST(asio_ctx(NewThreadExecutor{}), ctx);
  talk(ip, port).detach(ctx);
  static_cast<sys::IOContext*>(ctx->get_reserved())->run();
  return 0;
}
