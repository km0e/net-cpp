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

#include <span>
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

  std::shared_ptr<Context> ctx;
  std::thread poller_thread;

  void start_poller() {
    ctx = std::make_shared<Context>();
    poller_thread = std::thread([this] {
      ctx->run();
      log_debug("Poller shutdown");
    });
  }

  template <class Traits>
  void echo(AsyncSocket<Traits> &skt) {
    auto buf = std::make_unique<byte[]>(1024);
    for (auto &msg : echo_msg) {
      auto send_bytes = std::as_bytes(std::span(msg.data(), msg.size()));
      auto res = skt.write(send_bytes).block();
      ASSERT_TRUE(res);
      res = skt.read(buf.get(), 1024).block();
      ASSERT_TRUE(res) << "Read error: " << res.message();
      ASSERT_EQ(std::string_view(reinterpret_cast<char *>(buf.get()), res.size), msg);
    }
  }

  template <class Traits>
  void echo_to(AsyncSocket<Traits> &skt, SockAddr<Traits> &addr) {
    auto buf = std::make_unique<char[]>(1024);
    for (auto &msg : echo_msg) {
      auto send_bytes = std::as_bytes(std::span(msg.data(), msg.size()));
      auto res = skt.sendto(addr, send_bytes).block();
      ASSERT_TRUE(res);
      auto recv_bytes = std::as_writable_bytes(std::span(buf.get(), 1024));
      res = skt.recvfrom(addr, recv_bytes).block();
      ASSERT_TRUE(res);
      ASSERT_EQ(std::string_view(buf.get(), res.size), msg);
    }
  }

  void stop_poller() {
    ctx->shutdown();
    poller_thread.join();
    log_debug("Poller joined");
  }

public:
  AsyncSocketIOFixture() : ctx(nullptr), poller_thread() {}
};

TEST_F(AsyncSocketIOFixture, tcp_connect_with_ais) {
  auto res_skt = gai_async_connect<Tcp<Ip<4>>>(*ctx, ip.c_str(), port.c_str()).block();
  ASSERT_TRUE(res_skt.has_value());
  ASSERT_NE(res_skt->raw(), 0);
  echo(*res_skt);
  SockAddrCompose<Tcp<Ip<4>>> addr{ip, port};
  echo_to(*res_skt, addr);
}

// TEST_F(AsyncSocketIOFixture, tcp_connect_with_ip_port) {
//   auto skt = SocketCompose<Tcp<Ip<4>>>();
//   ASSERT_TRUE(skt.is_valid());
//   ASSERT_EQ(skt.connect({ip, port}), errc{});
//   auto async_skt = AsyncSocket(std::move(skt), *poller);
//   echo(async_skt);
//   SockAddrCompose<Tcp<Ip<4>>> addr{ip, port};
//   echo_to(async_skt, addr);
// }

TEST_F(AsyncSocketIOFixture, udp_connect_with_ais) {
  auto res = gai_connect<Udp<Ip<4>>>(ip.c_str(), port.c_str());
  ASSERT_TRUE(res.has_value());
  auto skt = AsyncSocket(*ctx, std::move(*res));
  echo(skt);
  SockAddrCompose<Udp<Ip<4>>> addr{ip, port};
  echo_to(skt, addr);
}

TEST_F(AsyncSocketIOFixture, udp_connect_with_ip_port) {
  auto skt = SocketCompose<Udp<Ip<4>>>();
  ASSERT_TRUE(skt.is_valid());
  ASSERT_TRUE(skt.connect({ip, port}));
  auto async_skt = AsyncSocket(*ctx, std::move(skt));
  echo(async_skt);
  SockAddrCompose<Udp<Ip<4>>> addr{ip, port};
  echo_to(async_skt, addr);
}

int main(int argc, char **argv) {
  CLI::App app{"TCP Client"};
  app.add_option("-i,--ip", ip, "Ip to connect to");
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);
  usleep(1000000);  // Give the server some time to start

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
