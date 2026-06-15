/**
 * @file connect.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <CLI/CLI.hpp>
#include <gtest/gtest.h>
#include <xsl/asio.h>
#include <xsl/feature.h>
#include <xsl/log.h>
#include <xsl/sys.h>

#include <string>
#include <string_view>
#include <thread>

using namespace xsl::asio;
using namespace xsl;
// there should have a echo server
std::string ip = "127.0.0.1";
std::string port = "12345";

using namespace xsl::sys;

const std::string_view echo_msg[] = {
    "Hello, World!\n",
    "This is a test message\n",
    "To test the echo server\n",
    "Goodbye!\n",
};

class AsyncSocketIOFixture : public ::testing::Test {
protected:
  void SetUp() override { start_poller(); }
  void TearDown() override { stop_poller(); }

  Rc<CoroContext> ctx;
  std::thread poller_thread;

  void start_poller() {
    MUST(asio_ctx(NewThreadExecutor{}), c);
    ctx = std::move(c);
    poller_thread = std::thread([this] {
      static_cast<IOContext*>(ctx->get_reserved())->run();
      log_debug("Poller shutdown");
    });
  }

  template <class AsyncSocket>
  void echo(AsyncSocket& skt) {
    auto buf = std::make_unique<byte[]>(1024);
    for (auto& msg : echo_msg) {
      auto send_bytes = std::as_bytes(std::span(msg.data(), msg.size()));
      auto res = skt->write(send_bytes).block();
      ASSERT_TRUE(res);
      auto recv_bytes = std::as_writable_bytes(std::span(buf.get(), 1024));
      res = skt->read(recv_bytes).block();
      ASSERT_TRUE(res) << "Read error: " << res.message();
      ASSERT_EQ(std::string_view(reinterpret_cast<char*>(buf.get()), res.size), msg);
    }
  }

  void stop_poller() {
    static_cast<IOContext*>(ctx->get_reserved())->shutdown();
    poller_thread.join();
    log_debug("Poller joined");
  }

public:
  AsyncSocketIOFixture() : ctx(), poller_thread() {}
};

TEST_F(AsyncSocketIOFixture, tcp_connect_with_ais) {
  auto util = AsyncSocketCreatorCompose<Tcp, Ip>();
  auto res_skt = util.ca2(ip.c_str(), port.c_str()).by(this->ctx).block();
  ASSERT_TRUE(res_skt.has_value());
  ASSERT_NE((*res_skt)->raw(), 0);
  echo(*res_skt);
}


TEST_F(AsyncSocketIOFixture, udp_connect_with_ip_port) {
  auto util = AsyncSocketCreatorCompose<Udp, Ip>();
  auto& ctx = *static_cast<sys::IOContext*>(this->ctx->get_reserved());
  auto res = util.c2(ctx, ip.c_str(), port.c_str());
  ASSERT_TRUE(res.has_value());
  echo(*res);
}

int main(int argc, char** argv) {
  CLI::App app{"TCP Client"};
  app.add_option("-i,--ip", ip, "Ip to connect to");
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);
  usleep(1000000);  // Give the server some time to start

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
