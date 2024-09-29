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

#include <forward_list>
#include <iostream>
#include <ostream>
#include <string>

// std::string ip = "8.8.8.8";
// std::string ip = "1.1.1.1";
std::string ip = "223.5.5.5";

std::string port = "53";

using namespace xsl::coro;
using namespace xsl;

Task<void> talk(std::string_view ip, std::string_view port, std::shared_ptr<xsl::Poller> poller) {
  auto cli = *dns::dial<Ip<4>>(ip.data(), port.data(), *poller);
  co_yield cli.run();
  std::string buffer(4096, '\0');
  while (true) {
    std::cin >> buffer;
    auto res = co_await cli.query(buffer);
    if (!res) {
      LOG5("Error: {}", to_string(res.error()));
      continue;
    }
    char ip[16];
    const std::forward_list<dns::RR> &rrs = **res;
    std::println(std::cout, "{:20}{:10}{:10}{:10}{:10}", "Name", "Type", "Class", "TTL", "RData");
    for (auto &rr : rrs) {
      std::print(std::cout, "{:20}{:10}{:10}{:10}", rr.name(), rr.type().to_string_view(),
                 rr.class_().to_string_view(), rr.ttl());
      if (rr.type() == dns::Type::A) {
        auto data = rr.rdata();
        snprintf(ip, 16, "%d.%d.%d.%d", data[0], data[1], data[2], data[3]);
        std::println(std::cout, "{}", ip);
      } else if (rr.type() == dns::Type::CNAME) {
        std::println(std::cout, "{}", std::string_view(reinterpret_cast<const char *>(rr.rdata().data())));
      }
    }
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
  // auto executor = std::make_shared<NewThreadExecutor>();
  // talk(ip, port, poller).detach(std::move(executor));
  talk(ip, port, poller).detach();
  poller->run();
  return 0;
}
