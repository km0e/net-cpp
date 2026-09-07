/**
 * @file net.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Unit tests for asio net module (AsyncSocket, async_connect, etc.)
 * @version 0.1.0
 * @date 2026-06-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <gtest/gtest.h>
#include <xsl/asio.h>
#include <xsl/coro.h>
#include <xsl/io.h>
#include <xsl/log.h>
#include <xsl/sys.h>

#include <sys/socket.h>
#include <thread>

using namespace xsl;
using namespace xsl::asio;

/// Fixture: creates an IOContext with a dedicated poller thread
class AsyncSocketTest : public ::testing::Test {
protected:
  void SetUp() override {
    MUST(asio_ctx(ThreadPoolExecutor{4}), c);
    ctx = std::move(c);
    poller = std::thread([this] {
      static_cast<sys::IOContext*>(ctx->get_reserved())->run();
    });
  }

  void TearDown() override {
    static_cast<sys::IOContext*>(ctx->get_reserved())->shutdown();
    if (poller.joinable()) poller.join();
  }

  IOContext& io_ctx() { return *static_cast<sys::IOContext*>(ctx->get_reserved()); }

  Rc<CoroContext> ctx;
  std::thread poller;
};

/// Test: make_async_socket creates a valid AsyncSocket from a raw fd
TEST_F(AsyncSocketTest, make_async_socket_from_fd) {
  int fds[2];
  ASSERT_NE(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), -1) << strerror(errno);

  auto sock = sys::net::Socket<sys::net::TcpIpv4SocketTraits>(fds[0]);
  auto res = make_async_socket(io_ctx(), std::move(sock));
  ASSERT_TRUE(res.has_value()) << std::make_error_code(res.error()).message();

  ::close(fds[1]);
}

/// Test: basic read/write through socketpair-wrapped AsyncSocket
TEST_F(AsyncSocketTest, async_socket_read_write) {
  int fds[2];
  ASSERT_NE(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), -1) << strerror(errno);

  auto s1 = sys::net::Socket<sys::net::TcpIpv4SocketTraits>(fds[0]);
  auto s2 = sys::net::Socket<sys::net::TcpIpv4SocketTraits>(fds[1]);

  auto r1 = make_async_socket(io_ctx(), std::move(s1));
  ASSERT_TRUE(r1.has_value());
  auto r2 = make_async_socket(io_ctx(), std::move(s2));
  ASSERT_TRUE(r2.has_value());

  auto skt_a = std::move(*r1);
  auto skt_b = std::move(*r2);

  const char* msg = "hello socketpair";
  auto send_bytes = std::as_bytes(std::span(msg, strlen(msg)));

  auto write_res = skt_a->write(send_bytes).block();
  ASSERT_TRUE(write_res) << write_res.message();
  ASSERT_EQ(write_res.size, strlen(msg));

  char buf[256]{};
  auto recv_bytes = std::as_writable_bytes(std::span(buf, sizeof(buf)));
  auto read_res = skt_b->read(recv_bytes).block();
  ASSERT_TRUE(read_res) << read_res.message();
  ASSERT_EQ(read_res.size, strlen(msg));
  ASSERT_EQ(std::string_view(buf, read_res.size), msg);
}

/// Test: make_async_socket fails with invalid fd
TEST_F(AsyncSocketTest, make_async_socket_bad_fd) {
  auto sock = sys::net::Socket<sys::net::TcpIpv4SocketTraits>(-1);
  auto res = make_async_socket(io_ctx(), std::move(sock));
  EXPECT_FALSE(res.has_value());
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
