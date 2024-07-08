#include "xsl/coro/task.h"
#include "xsl/logctl.h"
#include "xsl/net/sync.h"
#include "xsl/net/transport/resolve.h"
#include "xsl/net/transport/tcp.h"

#include <CLI/CLI.hpp>
#include <gtest/gtest.h>

#include <string>
#include <thread>
using namespace xsl::coro;
// there should have a echo server
std::string host = "127.0.0.1";
std::string port = "12347";

TEST(connect, connect) {
  using namespace xsl::net::transport::tcp;
  using namespace xsl::net::transport;
  using namespace xsl;
  auto res = Resolver<feature::ip<4>, feature::tcp>::resolve(host.c_str(), port.c_str());
  ASSERT_TRUE(res.has_value());
  auto ai = std::move(res.value());
  auto poller = std::make_shared<net::Poller>();
  std::thread t([&poller]() {
    while (poller->valid()) {
      poller->poll();
    }
    DEBUG("Poller shutdown");
  });
  auto sock = connect(ai, poller);
  auto skt = sock.block();
  EXPECT_TRUE(skt.has_value());
  EXPECT_NE(skt.value().raw_fd(), 0);
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
