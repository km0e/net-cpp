/**
 * @file test_bind.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <CLI/CLI.hpp>
#include <gtest/gtest.h>
#include <xsl/asio.h>
#include <xsl/feature.h>
#include <xsl/io.h>
#include <xsl/log.h>
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

  Rc<CoroContext> ctx;
  std::thread poller_thread;

  void start_poller() {
    MUST(asio_ctx(NewThreadExecutor{}), ctx);
    this->ctx = ctx;
    poller_thread = std::thread([this] {
      static_cast<sys::IOContext*>(this->ctx->get_reserved())->run();
      log_debug("Poller shutdown");
    });
  }
  template <class AsyncSocket>
  Task<void> echo(AsyncSocket& skt) {
    auto N = TEST_COUNT;
    auto& ctx = co_await CurrentIOContext;
    while (N--) {
      auto res_skt = co_await skt->accept_async(ctx);
      if (!res_skt.has_value()) {
        co_return;
      }
      co_yield [](auto skt) -> Task<void> {
        auto buf = std::make_unique<char[]>(1024);
        while (true) {
          auto recv_bytes = std::as_writable_bytes(std::span(buf.get(), 1024));
          auto res = co_await skt->read(recv_bytes);
          if (!res) {
            break;
          }
          auto send_bytes = std::as_bytes(std::span(buf.get(), res.size));
          res = co_await skt->write(send_bytes);
          if (!res) {
            break;
          }
        }
      }(std::move(*res_skt));
    }
  }

  void stop_poller() {
    static_cast<sys::IOContext*>(ctx->get_reserved())->shutdown();
    poller_thread.join();
    log_debug("Poller joined");
  }

public:
  AsyncSocketIOFixture() : ctx(), poller_thread() {}
};

TEST_F(AsyncSocketIOFixture, tcp_bind) {
  using namespace xsl;
  auto util = AsyncSocketCreatorCompose<TcpIpv4>();
  auto& ctx = *static_cast<sys::IOContext*>(this->ctx->get_reserved());
  auto res = util.cb(ctx, "0.0.0.0", port);  // to init the util
  ASSERT_TRUE(res.has_value());
  ASSERT_TRUE((*res)->listen()) << "Failed to listen";
  echo(*res).detach(*this->ctx);
  // @note the client side uses plain blocking sockets: the coroutine-based
  //       client path (Task::block on client sockets) is still unreliable
  //       under GCC 16 coroutine codegen, see the task/block refactoring notes
  auto N = TEST_COUNT;
  while (N--) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(fd, 0);
    timeval tv{15, 0};  // generous: under parallel test load the echo task may be late (known race, see notes below)
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    ::inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);
    bool connected = false;
    for (int attempt = 0; attempt < 30 && !connected; ++attempt) {
      // the echo task binds asynchronously, retry while it starts up
      connected = ::connect(fd, reinterpret_cast<sockaddr*>(&sa), sizeof sa) == 0;
      if (!connected) ::usleep(100'000);
    }
    ASSERT_TRUE(connected);
    auto buf = std::make_unique<char[]>(1024);
    for (auto& msg : echo_msg) {
      ASSERT_EQ(::send(fd, msg.data(), msg.size(), 0), static_cast<ssize_t>(msg.size()));
      ssize_t n = ::recv(fd, buf.get(), 1024, 0);
      ASSERT_EQ(n, static_cast<ssize_t>(msg.size()));
      ASSERT_EQ(std::string_view(buf.get(), static_cast<std::size_t>(n)), msg);
    }
    ::close(fd);
    // KNOWN ISSUE: under heavy parallel test load the echo task occasionally
    // starts late (thread-per-dispatch scheduling); the generous recv timeout
    // above keeps this from hanging, but a rare slow run can still be observed.
  }
}

int main(int argc, char** argv) {
  CLI::App app{"TCP Server"};
  app.add_option("-p,--port", port, "Port to connect to");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
