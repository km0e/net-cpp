/**
 * @file test_connect.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/feature.h"
#include "xsl/logctl.h"
#include "xsl/net.h"
#include "xsl/sys.h"

#include <CLI/CLI.hpp>
#include <gtest/gtest.h>

#include <cstdlib>
#include <string>
#include <thread>
using namespace xsl::coro;
using namespace xsl;
// there should have a echo server
std::string host = "127.0.0.1";
std::string port = "12347";

TEST(connect, connect) {
  using namespace xsl::sys::net;
  using namespace xsl;
  auto res = getaddrinfo<Tcp<Ip<4>>>(host.c_str(), port.c_str());
  ASSERT_TRUE(res.has_value());
  auto ai = std::move(res.value());
  auto poller = std::make_shared<Poller>();
  std::thread t([&poller] {
    while (poller->valid()) {
      poller->poll();
    }
    LOG5("Poller shutdown");
  });
  auto skt = sys::net::connect(ai, *poller).block();
  LOG5("Socket connected");
  xsl::flush_log();
  ASSERT_TRUE(skt.has_value());
  ASSERT_NE(skt->raw(), 0);
  poller->shutdown();
  t.join();
  LOG5("Poller joined");
  xsl::flush_log();
}

int main(int argc, char **argv) {
  // xsl::set_log_level(xsl::LogLevel::DEBUG);
  xsl::no_log();

  CLI::App app{"TCP Client"};
  app.add_option("-i,--ip", host, "Ip to connect to");
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
