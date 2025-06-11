/**
 * @file test_bind.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <CLI/CLI.hpp>
#include <gtest/gtest.h>
#include <xsl/asio.h>
#include <xsl/feature.h>
#include <xsl/io.h>
#include <xsl/logctl.h>
#include <xsl/sys.h>

#include <cstdint>
#include <string>
using namespace xsl;
using namespace xsl::asio;
// there should have a echo server
uint16_t port = 12348;

std::size_t TEST_COUNT = 1;

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
      log_debug("Poller shutdown");
    });
  }
  template <class Traits>
  Task<void> echo(AsyncSocket<Traits> &skt) {
    auto N = TEST_COUNT;
    while (N--) {
      auto res_skt = co_await skt.accept();
      if (!res_skt.has_value()) {
        co_return;
      }
      co_yield [](auto skt) -> Task<void> {
        auto buf = std::make_unique<char[]>(1024);
        while (true) {
          auto recv_bytes = std::as_writable_bytes(std::span(buf.get(), 1024));
          auto [m, r_err] = co_await skt.read(recv_bytes);
          if (r_err) {
            break;
          }
          auto send_bytes = std::as_bytes(std::span(buf.get(), m));
          auto [n, s_err] = co_await skt.write(send_bytes);
          if (s_err) {
            break;
          }
        }
      }(AsyncSocket(std::move(*res_skt), *poller));
    }
  }

  void stop_poller() {
    poller->shutdown();
    poller_thread.join();
    log_debug("Poller joined");
  }

public:
  AsyncSocketIOFixture() : poller(nullptr), poller_thread() {}
};

TEST_F(AsyncSocketIOFixture, tcp_bind) {
  using namespace xsl;
  auto res_skt = net::gai_bind<TcpIpv4>(port);
  if (!res_skt.has_value()) {
    log_debug("Failed to bind: {}", res_skt.error().message());
  }
  ASSERT_EQ(res_skt->listen(), errc{}) << "Failed to listen";
  ASSERT_TRUE(res_skt.has_value());
  auto skt = AsyncSocket(std::move(*res_skt), *poller);
  echo(skt).detach();
  xsl::flush_log();
  auto N = TEST_COUNT;
  while (N--) {
    // auto res_client = getaddrinfo<TcpIpv4>("127.0.0.1", port);
    auto res_client = gai_async_connect<TcpIpv4>(*poller, "127.0.0.1", port).block();
    ASSERT_TRUE(res_client.has_value());
    auto client = std::move(*res_client);
    auto buf = std::make_unique<char[]>(1024);
    for (auto &msg : echo_msg) {
      auto send_bytes = std::as_bytes(std::span(msg.data(), msg.size()));
      auto [n, s_err] = client.write(send_bytes).block();
      ASSERT_FALSE(s_err);
      auto recv_bytes = std::as_writable_bytes(std::span(buf.get(), 1024));
      auto [m, r_err] = client.read(recv_bytes).block();
      ASSERT_FALSE(r_err);
      ASSERT_EQ(std::string_view(buf.get(), m), msg);
    }
  }
}

int main(int argc, char **argv) {
  CLI::App app{"TCP Server"};
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
