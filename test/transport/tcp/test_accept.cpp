#include "xsl/coro/block.h"
#include "xsl/feature.h"
#include "xsl/logctl.h"
#include "xsl/net/transport.h"

#include <CLI/CLI.hpp>
#include <gtest/gtest.h>

#include <cstdint>
#include <semaphore>
#include <string>
using namespace xsl::coro;
using namespace xsl;
// there should have a echo server
uint16_t port = 12350;

TEST(bind, create) {
  using namespace xsl::net;
  using namespace xsl::feature;
  auto res = Resolver{}.resolve<Ip<4>, Tcp>(port);
  ASSERT_TRUE(res.has_value());
  auto ai = std::move(res.value());
  auto skt = bind(ai);
  ASSERT_TRUE(skt.has_value());
  ASSERT_NE(skt.value().raw_fd(), 0);
  ASSERT_TRUE(listen(skt.value()).has_value());
  auto poller = std::make_shared<Poller>();
  std::binary_semaphore con(0);
  std::binary_semaphore acc(0);
  std::thread t([&poller]() {
    while (poller->valid()) {
      poller->poll();
    }
    INFO("Poller shutdown");
  });
  bool ok = false;
  xsl::flush_log();
  std::thread t2([&poller, &ok, &con, &acc]() {
    auto res = Resolver{}.resolve<Ip<4>, Tcp>("127.0.0.1", port);
    if (!res.has_value()) {
      INFO("Failed to resolve server address");
      con.release();
      return;
    }
    auto con_res = connect(res.value(), poller);
    auto skt = con_res.block();
    if (!skt.has_value()) {
      INFO("Failed to connect to server");
      con.release();
      return;
    }
    ok = true;
    con.release();
    INFO("Connected to server");
    acc.acquire();
  });
  con.acquire();
  ASSERT_TRUE(ok);
  xsl::flush_log();
  auto acceptor = Acceptor{std::move(skt.value()), poller};
  auto task = acceptor.accept();
  auto accept_res = block(task);
  acc.release();
  auto [skt2, addr] = std::move(accept_res);
  ASSERT_NE(skt2.raw_fd(), 0);
  poller->shutdown();
  t.join();
  t2.join();
  xsl::flush_log();
  INFO("Poller joined");
}

int main(int argc, char **argv) {
  xsl::no_log();

  CLI::App app{"TCP Server"};
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
