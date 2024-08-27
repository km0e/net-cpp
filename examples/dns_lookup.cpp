/**
 * @file dns_lookup.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS lookup
 * @version 0.1
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

#include <iostream>
#include <string>

std::string ip = "58.20.127.238";
std::string port = "53";

using namespace xsl::coro;
using namespace xsl;

template <class Executor = ExecutorBase>
Task<void, Executor> talk(std::string_view ip, std::string_view port,
                          std::shared_ptr<xsl::Poller> poller) {
  auto server = dns::serv<Ip<4>>(ip.data(), port.data(), *poller).value();
  std::string buffer(4096, '\0');
  while (true) {
    std::cin >> buffer;
    auto res = co_await server.query<Executor>(buffer);
    if (res.empty()) {
      LOG5("Error: invalid domain name");
      continue;
    }
    LOG4("Received: {}", res);
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
  talk<NewThreadExecutor>(ip, port, poller).detach(std::move(executor));
  // echo(ip, port, poller).detach();
  while (true) {
    poller->poll();
  }
  return 0;
}
