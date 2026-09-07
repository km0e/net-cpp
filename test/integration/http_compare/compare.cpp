/**
 * @file compare.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Correctness comparison between xsl::asio HTTP server and a standalone
 *        asio HTTP server serving the same routes
 * @version 0.1.0
 * @date 2025-12-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <gtest/gtest.h>

#include <http_bench/asio_server.h>
#include <http_bench/xsl_server.h>
#include <xsl/asio.h>
#include <xsl/log.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace {

/// @brief plain blocking HTTP client, no external dependency
class BlockingHttpClient {
public:
  /// @brief connect to 127.0.0.1:port, retrying while the server starts up
  static bool connect(int& fd, std::uint16_t port, int attempts = 50) {
    for (int i = 0; i < attempts; ++i) {
      fd = ::socket(AF_INET, SOCK_STREAM, 0);
      if (fd < 0) return false;
      sockaddr_in sa{};
      sa.sin_family = AF_INET;
      sa.sin_port = htons(port);
      ::inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);
      if (::connect(fd, reinterpret_cast<sockaddr*>(&sa), sizeof sa) == 0) {
        int one = 1;
        ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
        timeval tv{5, 0};  // never hang forever on a broken server
        ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
        ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
        return true;
      }
      ::close(fd);
      fd = -1;
      ::usleep(100'000);
    }
    return false;
  }

  static void disconnect(int fd) {
    if (fd >= 0) ::close(fd);
  }

  /// @brief send a raw request and read one complete response
  /// @param body_read_until_close used when no Content-Length is present
  static bool request(int fd, std::string_view raw, std::string& status,
                      std::map<std::string, std::string>& headers, std::string& body,
                      bool* saw_eof = nullptr) {
    if (!write_all(fd, raw)) return false;
    std::string buf;
    buf.reserve(4096);
    char chunk[8192];
    while (buf.find("\r\n\r\n") == std::string::npos) {
      ssize_t got = ::recv(fd, chunk, sizeof chunk, 0);
      if (got <= 0) return false;
      buf.append(chunk, static_cast<std::size_t>(got));
    }
    std::size_t head_end = buf.find("\r\n\r\n") + 4;
    std::string_view head(buf.data(), head_end);
    std::size_t line_end = head.find("\r\n");
    status = std::string(head.substr(0, line_end));
    std::size_t pos = line_end + 2;
    while (pos != std::string_view::npos && pos < head.size()) {
      std::size_t end = head.find("\r\n", pos);
      std::string_view line = head.substr(pos, end == std::string_view::npos
                                                   ? head.size() - pos
                                                   : end - pos);
      std::size_t colon = line.find(':');
      if (colon != std::string_view::npos) {
        std::string key(line.substr(0, colon));
        std::string_view value = line.substr(colon + 1);
        value.remove_prefix(std::min(value.size(), value.find_first_not_of(" \t")));
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        headers[key] = std::string(value);
      }
      pos = end == std::string_view::npos ? std::string_view::npos : end + 2;
    }
    // body
    if (auto iter = headers.find("content-length"); iter != headers.end()) {
      std::size_t length = static_cast<std::size_t>(std::strtoull(iter->second.c_str(), nullptr, 10));
      body.assign(buf, head_end, std::string::npos);
      while (body.size() < length) {
        ssize_t got = ::recv(fd, chunk, sizeof chunk, 0);
        if (got <= 0) return false;
        body.append(chunk, static_cast<std::size_t>(got));
      }
      body.resize(length);
      return true;
    }
    // no Content-Length: the response is delimited by connection close
    body.assign(buf, head_end, std::string::npos);
    ssize_t got;
    while ((got = ::recv(fd, chunk, sizeof chunk, 0)) > 0) {
      body.append(chunk, static_cast<std::size_t>(got));
    }
    if (saw_eof != nullptr) *saw_eof = got == 0;
    return true;
  }

private:
  static bool write_all(int fd, std::string_view data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
      ssize_t n = ::send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
      if (n <= 0) return false;
      sent += static_cast<std::size_t>(n);
    }
    return true;
  }
};

constexpr std::uint16_t kXslPort = 18080;
constexpr std::uint16_t kAsioPort = 18081;

class HttpCompare : public ::testing::Test {
protected:
  void SetUp() override {
    // xsl server: coroutine context + poller thread, same pattern as examples
    MUST(xsl::asio::asio_ctx(xsl::coro::ThreadPoolExecutor{4}), ctx);
    this->ctx = ctx;
    this->poller = std::thread([this] {
      static_cast<xsl::sys::IOContext*>(this->ctx->get_reserved())->run();
    });
    http_bench::run_xsl_hello_server("127.0.0.1", kXslPort).detach(*ctx);

    // standalone asio server
    this->guard.emplace(asio::make_work_guard(this->ioc));
    this->asio_server.emplace(this->ioc, "127.0.0.1", kAsioPort);
    this->asio_thread = std::thread([this] { this->ioc.run(); });
  }

  void TearDown() override {
    static_cast<xsl::sys::IOContext*>(this->ctx->get_reserved())->shutdown();
    this->poller.join();
    this->ioc.stop();
    this->asio_thread.join();
    this->guard.reset();
  }

  /// @brief run one GET against both servers and compare the meaningful parts
  void compare_servers(std::string_view path, std::string_view extra_headers,
                       const std::string& expect_status, const std::string& expect_body,
                       bool expect_close) {
    for (auto [name, port] : {std::pair{"xsl", kXslPort}, {"asio", kAsioPort}}) {
      int fd = -1;
      ASSERT_TRUE(BlockingHttpClient::connect(fd, port)) << name << ": connect failed";
      std::string raw = std::format(
          "GET {} HTTP/1.1\r\nHost: 127.0.0.1\r\nUser-Agent: it-http-compare/0.1\r\n{}"
          "\r\n",
          path, extra_headers);
      std::string status;
      std::map<std::string, std::string> headers;
      std::string body;
      bool saw_eof = false;
      ASSERT_TRUE(BlockingHttpClient::request(fd, raw, status, headers, body, &saw_eof))
          << name << ": request failed";
      EXPECT_EQ(status, expect_status) << name << ": status line mismatch";
      if (!expect_body.empty()) {
        EXPECT_EQ(headers.at("content-length"), std::to_string(expect_body.size()))
            << name << ": content-length mismatch";
      }
      EXPECT_EQ(body, expect_body) << name << ": body mismatch";
      EXPECT_FALSE(headers.find("date") == headers.end()) << name << ": missing Date header";
      if (expect_close) {
        EXPECT_EQ(headers.at("connection"), "close")
            << name << ": server should acknowledge the close";
        if (headers.find("content-length") == headers.end()) {
          // no Content-Length: the body must be delimited by EOF
          EXPECT_TRUE(saw_eof) << name << ": connection should be closed by the server";
        }
      }
      BlockingHttpClient::disconnect(fd);
    }
  }

  xsl::Rc<xsl::CoroContext> ctx;
  std::thread poller;
  asio::io_context ioc;
  std::optional<asio::executor_work_guard<asio::io_context::executor_type>> guard;
  std::optional<http_bench::AsioHelloServer> asio_server;
  std::thread asio_thread;
};

TEST_F(HttpCompare, hello_ok) {
  this->compare_servers(http_bench::HELLO_PATH, "", "HTTP/1.1 200 OK",
                        std::string(http_bench::HELLO_BODY), false);
}

TEST_F(HttpCompare, not_found_close) {
  this->compare_servers("/missing", "Connection: close\r\n", "HTTP/1.1 404 Not Found", "", true);
}

TEST_F(HttpCompare, keep_alive_three_requests) {
  for (auto port : {kXslPort, kAsioPort}) {
    int fd = -1;
    ASSERT_TRUE(BlockingHttpClient::connect(fd, port));
    for (int i = 0; i < 3; ++i) {
      std::string raw = std::format(
          "GET {} HTTP/1.1\r\nHost: 127.0.0.1\r\nUser-Agent: it-http-compare/0.1\r\n\r\n",
          http_bench::HELLO_PATH);
      std::string status;
      std::map<std::string, std::string> headers;
      std::string body;
      ASSERT_TRUE(BlockingHttpClient::request(fd, raw, status, headers, body))
          << "port " << port << ": request " << i << " failed";
      EXPECT_EQ(status, "HTTP/1.1 200 OK");
      EXPECT_EQ(body, std::string(http_bench::HELLO_BODY));
    }
    BlockingHttpClient::disconnect(fd);
  }
}

TEST_F(HttpCompare, concurrent_requests) {
  constexpr int kThreads = 4;
  constexpr int kRequests = 25;
  for (auto [name, port] : {std::pair{"xsl", kXslPort}, {"asio", kAsioPort}}) {
    std::atomic<int> failures{0};
    std::vector<std::thread> workers;
    for (int t = 0; t < kThreads; ++t) {
      workers.emplace_back([port, &failures] {
        for (int i = 0; i < kRequests; ++i) {
          int fd = -1;
          if (!BlockingHttpClient::connect(fd, port)) {
            ++failures;
            continue;
          }
          std::string raw = std::format(
              "GET {} HTTP/1.1\r\nHost: 127.0.0.1\r\nUser-Agent: it-http-compare/0.1\r\n\r\n",
              http_bench::HELLO_PATH);
          std::string status;
          std::map<std::string, std::string> headers;
          std::string body;
          if (!BlockingHttpClient::request(fd, raw, status, headers, body) || status != "HTTP/1.1 200 OK"
              || body != std::string(http_bench::HELLO_BODY)) {
            ++failures;
          }
          BlockingHttpClient::disconnect(fd);
        }
      });
    }
    for (auto& worker : workers) worker.join();
    EXPECT_EQ(failures.load(), 0) << name << ": concurrent requests failed";
  }
}

}  // namespace

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
