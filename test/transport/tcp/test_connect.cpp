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
  ASSERT_TRUE(res.is_ok());
  auto ai = res.unwrap();
  auto poller = std::make_shared<net::DefaultPoller>();
  std::thread t([&poller]() {
    while (poller->valid()) {
      poller->poll();
    }
    DEBUG("Poller shutdown");
  });
  auto sock = connect(ai, poller);
  auto skt = sock.block();
  EXPECT_TRUE(skt.is_ok());
  // EXPECT_NE(skt.unwrap().raw_fd(), 0);
  poller->shutdown();
  t.join();
}

int main(int argc, char **argv) {
  xsl::no_log();
  // xsl::set_log_level(xsl::LogLevel::TRACE);

  CLI::App app{"TCP Client"};
  app.add_option("-i,--ip", host, "Ip to connect to");
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
