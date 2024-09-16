/**
 * @file test_connect.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/feature.h"
#include "xsl/logctl.h"
#include "xsl/sys.h"
#include "xsl/wheel.h"

#include <CLI/CLI.hpp>
#include <gtest/gtest.h>

#include <cstdlib>
#include <string>
#include <string_view>
#include <thread>
using namespace xsl::coro;
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

  std::shared_ptr<Poller> poller;
  std::thread poller_thread;

  void start_poller() {
    poller = std::make_shared<Poller>();
    poller_thread = std::thread([this] {
      poller->run();
      LOG5("Poller shutdown");
    });
  }

  template <class Traits>
  void echo(net::AsyncReadWriteSocket<Traits> &skt) {
    auto buf = std::make_unique<char[]>(1024);
    for (auto &msg : echo_msg) {
      auto send_bytes = xsl::as_bytes(msg.data(), msg.size());
      auto [n, s_err] = skt.send(send_bytes).block();
      ASSERT_FALSE(s_err);
      auto recv_bytes = xsl::as_writable_bytes(buf.get(), 1024);
      auto [m, r_err] = skt.recv(recv_bytes).block();
      ASSERT_FALSE(r_err);
      ASSERT_EQ(std::string_view(buf.get(), m), msg);
    }
  }

  template <class Traits>
  void echo_to(net::AsyncReadWriteSocket<Traits> &skt, net::SockAddr<Traits> &addr) {
    auto buf = std::make_unique<char[]>(1024);
    for (auto &msg : echo_msg) {
      auto send_bytes = xsl::as_bytes(msg.data(), msg.size());
      auto [n, s_err] = skt.sendto(send_bytes, addr).block();
      ASSERT_FALSE(s_err);
      auto recv_bytes = xsl::as_writable_bytes(buf.get(), 1024);
      auto [m, r_err] = skt.recvfrom(recv_bytes, addr).block();
      ASSERT_FALSE(r_err);
      ASSERT_EQ(std::string_view(buf.get(), m), msg);
    }
  }

  void stop_poller() {
    poller->shutdown();
    poller_thread.join();
    LOG5("Poller joined");
  }

public:
  AsyncSocketIOFixture() : poller(nullptr), poller_thread() {}
};

TEST_F(AsyncSocketIOFixture, tcp_connect_with_ais) {
  auto res_skt = net::gai_async_connect<Tcp<Ip<4>>>(*poller, ip.c_str(), port.c_str()).block();
  ASSERT_TRUE(res_skt.has_value());
  ASSERT_NE(res_skt->raw(), 0);
  echo(*res_skt);
  net::SockAddrCompose<Tcp<Ip<4>>> addr{ip, port};
  echo_to(*res_skt, addr);
}

TEST_F(AsyncSocketIOFixture, tcp_connect_with_ip_port) {
  auto skt = net::Socket<Tcp<Ip<4>>>();
  ASSERT_TRUE(skt.is_valid());
  auto res_async_skt = std::move(skt).async_connect({ip, port}, *poller).block();
  ASSERT_TRUE(res_async_skt.has_value());
  echo(*res_async_skt);
  net::SockAddrCompose<Tcp<Ip<4>>> addr{ip, port};
  echo_to(*res_async_skt, addr);
}

TEST_F(AsyncSocketIOFixture, udp_connect_with_ais) {
  auto res = net::gai_connect<Udp<Ip<4>>>(ip.c_str(), port.c_str());
  ASSERT_TRUE(res.has_value());
  auto skt = std::move(*res).async(*poller);
  echo(skt);
  net::SockAddrCompose<Udp<Ip<4>>> addr{ip, port};
  echo_to(skt, addr);
}

TEST_F(AsyncSocketIOFixture, udp_connect_with_ip_port) {
  auto skt = net::Socket<Udp<Ip<4>>>();
  ASSERT_TRUE(skt.is_valid());
  ASSERT_EQ(skt.connect({ip, port}), errc{});
  auto async_skt = std::move(skt).async(*poller);
  echo(async_skt);
  net::SockAddrCompose<Udp<Ip<4>>> addr{ip, port};
  echo_to(async_skt, addr);
}

int main(int argc, char **argv) {
  CLI::App app{"TCP Client"};
  app.add_option("-i,--ip", ip, "Ip to connect to");
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
