#include "xsl/logctl.h"
#include "xsl/net/transport/resolve.h"
#include "xsl/net/transport/tcp.h"

#include <CLI/CLI.hpp>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
using namespace xsl::coro;
// there should have a echo server
uint16_t port = 12349;

TEST(bind, create) {
  using namespace xsl::net::transport::tcp;
  using namespace xsl::net::transport;
  using namespace xsl;
  auto res = Resolver<feature::ip<4>, feature::tcp>::resolve(port);
  ASSERT_TRUE(res.has_value());
  auto ai = std::move(res.value());
  auto skt = bind(ai);
  ASSERT_TRUE(skt.has_value());
  ASSERT_NE(skt.value().raw_fd(), 0);
  ASSERT_TRUE(listen(skt.value()).has_value());
}

int main(int argc, char **argv) {
  xsl::no_log();

  CLI::App app{"TCP Server"};
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
