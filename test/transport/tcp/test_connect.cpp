#include "xsl/feature.h"
#include "xsl/logctl.h"
#include "xsl/sync.h"
#include "xsl/sys.h"

#include <CLI/CLI.hpp>
#include <gtest/gtest.h>

#include <string>
#include <thread>
using namespace xsl::coro;
using namespace xsl;
// there should have a echo server
std::string host = "127.0.0.1";
std::string port = "12347";

TEST(connect, connect) {
  using namespace xsl::net;
  using namespace xsl::feature;
  auto res = Resolver{}.resolve<Ip<4>, Tcp>(host.c_str(), port.c_str());
  ASSERT_TRUE(res.has_value());
  auto ai = std::move(res.value());
  auto poller = std::make_shared<Poller>();
  std::thread t([&poller]() {
    while (poller->valid()) {
      poller->poll();
    }
    DEBUG("Poller shutdown");
  });
  auto sock = connect(ai, poller);
  auto skt = sock.block();
  ASSERT_TRUE(skt.has_value());
  ASSERT_NE(skt.value().raw(), 0);
  poller->shutdown();
  t.join();
}

int main(int argc, char **argv) {
  xsl::no_log();

  CLI::App app{"TCP Client"};
  app.add_option("-i,--ip", host, "Ip to connect to");
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
