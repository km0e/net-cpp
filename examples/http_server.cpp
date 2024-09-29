/**
 * @file http_server.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <CLI/CLI.hpp>
#include <xsl/coro.h>
#include <xsl/logctl.h>
#include <xsl/net.h>

std::string ip = "0.0.0.0";
std::string port = "8080";

using namespace xsl::coro;
using namespace xsl;
/**
 * @brief run http server
 *
 * @param ip
 * @param port
 * @param poller
 * @return Lazy<void>
 * @note this example all use dynamic call, you can also find some
 */
Task<void> run(std::string_view ip, std::string_view port, std::shared_ptr<xsl::Poller> poller) {
  auto http_server = http1::Server{tcp::make_server<Ip<4>>(ip, port, poller).value()};
  auto service = http1::make_service();
  service.redirect(http::Method::GET, "/", "/index.html");
  service.add_static("/", {"./build/html/", {"br"}});
  co_await http_server.serve_connection(std::move(service).build());
  poller->shutdown();
  co_return;
}

int main(int argc, char* argv[]) {
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);

  auto poller = std::make_shared<xsl::Poller>();
  auto executor = std::make_shared<NewThreadExecutor>();
  run(ip, port, poller).detach(std::move(executor));
  // run(ip, port, poller).detach();
  poller->run();
  return 0;
}
