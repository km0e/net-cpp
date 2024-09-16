/**
 * @file udp_echo.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief UDP echo server
 * @version 0.11
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <CLI/CLI.hpp>
#include <xsl/coro.h>
#include <xsl/feature.h>
#include <xsl/logctl.h>
#include <xsl/net.h>
#include <xsl/sys.h>
#include <xsl/wheel.h>

#include <span>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl::coro;
using namespace xsl;

Task<void> talk(std::string_view ip, std::string_view port, std::shared_ptr<xsl::Poller> poller) {
  std::string buffer(4096, '\0');
  auto rw =net::gai_bind<UdpIpv4>(ip.data(), port.data()).value().async(*poller);
  sys::net::SockAddrCompose<UdpIpv4> addr{};
  std::string dst(128, '\0');
  std::uint16_t port_num;
  while (true) {
    auto [rc_n, rc_err] = co_await rw.recvfrom(xsl::as_writable_bytes(std::span(buffer)), addr);
    if (rc_err) {
      LOG5("Error: {}", std::make_error_code(rc_err.value()).message());
      break;
    }
    if (auto res = addr.parse(dst, port_num); res != errc{}) {
      LOG5("Error: {}", to_string(res));
      continue;
    }
    LOG4("Received: {} from {}:{}", std::string_view{buffer.data(), rc_n}, dst, port_num);
    auto [sd_n, sd_err]
        = co_await rw.sendto(xsl::as_bytes(std::span(buffer).subspan(0, rc_n)), addr);
    if (sd_err) {
      LOG5("Error: {}", std::make_error_code(sd_err.value()).message());
      break;
    }
    LOG4("Sent: {}", std::string_view{buffer.data(), sd_n});
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
  // talk(ip, port, poller).detach();
  poller->run();
  return 0;
}
