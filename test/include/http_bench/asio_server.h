/**
 * @file asio_server.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Minimal HTTP/1.1 hello server built on standalone asio, used to
 *        compare with xsl::asio::HttpServer (see test/benches/http and
 *        test/integration/http_compare)
 * @version 0.1.0
 * @date 2025-12-17
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef HTTP_BENCH_ASIO_SERVER
#  define HTTP_BENCH_ASIO_SERVER
#  include <asio.hpp>

#  include <http_bench/common.h>

#  include <algorithm>
#  include <chrono>
#  include <cstddef>
#  include <cstdint>
#  include <format>
#  include <optional>
#  include <string>
#  include <string_view>

namespace http_bench {

/// @brief current date in the same format xsl uses (std::format {:%a, %d %b %Y %T %Z})
inline std::string http_date_now() {
  return std::format("{:%a, %d %b %Y %T %Z}",
                     std::chrono::time_point_cast<std::chrono::seconds>(
                         std::chrono::system_clock::now()));
}

/// @brief parsed request head
struct HttpRequestView {
  std::string_view method;
  std::string_view target;
  bool keep_alive = true;
  std::size_t content_length = 0;
};

/**
 * @brief parse a request head block (including the trailing "\r\n\r\n")
 *
 * @return std::nullopt if the request line is malformed
 */
inline std::optional<HttpRequestView> parse_request_head(std::string_view raw) {
  std::size_t head_end = raw.find("\r\n\r\n");
  if (head_end == std::string_view::npos) return std::nullopt;
  std::string_view head = raw.substr(0, head_end);
  HttpRequestView result;
  std::size_t line_end = head.find("\r\n");
  std::string_view request_line
      = line_end == std::string_view::npos ? head : head.substr(0, line_end);
  std::size_t sp1 = request_line.find(' ');
  std::size_t sp2 = request_line.find(' ', sp1 == std::string_view::npos ? 0 : sp1 + 1);
  if (sp1 == std::string_view::npos || sp2 == std::string_view::npos) return std::nullopt;
  result.method = request_line.substr(0, sp1);
  result.target = request_line.substr(sp1 + 1, sp2 - sp1 - 1);
  std::string_view version = request_line.substr(sp2 + 1);
  result.keep_alive = version != "HTTP/1.0";
  auto equals_ignore_case = [](std::string_view lhs, std::string_view rhs) {
    return lhs.size() == rhs.size()
           && std::equal(lhs.begin(), lhs.end(), rhs.begin(),
                         [](char a, char b) { return std::tolower((unsigned char)a)
                                                    == std::tolower((unsigned char)b); });
  };
  std::size_t pos = line_end == std::string_view::npos ? std::string_view::npos : line_end + 2;
  while (pos != std::string_view::npos && pos < head.size()) {
    std::size_t end = head.find("\r\n", pos);
    std::string_view line = head.substr(pos, end == std::string_view::npos ? head.size() - pos
                                                                           : end - pos);
    std::size_t colon = line.find(':');
    if (colon != std::string_view::npos) {
      std::string_view key = line.substr(0, colon);
      std::string_view value = line.substr(colon + 1);
      value.remove_prefix(std::min(value.size(), value.find_first_not_of(" \t")));
      if (equals_ignore_case(key, "Content-Length")) {
        result.content_length = std::strtoull(std::string(value).c_str(), nullptr, 10);
      } else if (equals_ignore_case(key, "Connection")) {
        if (equals_ignore_case(value, "close")) result.keep_alive = false;
        if (equals_ignore_case(value, "keep-alive")) result.keep_alive = true;
      }
    }
    pos = end == std::string_view::npos ? std::string_view::npos : end + 2;
  }
  return result;
}

/// @brief build a response head + body, single write to keep the syscall count low
inline std::string build_http_response(int status, std::string_view body, bool close) {
  std::string_view reason = status == 200 ? "OK"
                            : status == 404 ? "Not Found"
                                            : "Bad Request";
  std::string head = std::format("HTTP/1.1 {} {}\r\nDate: {}\r\n", status, reason, http_date_now());
  if (status == 200) head += "Content-Type: text/plain\r\n";
  std::format_to(std::back_inserter(head), "Content-Length: {}\r\n", body.size());
  if (close) head += "Connection: close\r\n";
  head += "\r\n";
  head += body;
  return head;
}

/**
 * @brief a minimal keep-alive HTTP/1.1 server on standalone asio
 *
 * Routes mirror http_bench::run_xsl_hello_server:
 *   - GET /hello -> 200 "Hello, World!" (text/plain)
 *   - anything else -> 404
 */
class AsioHelloServer {
public:
  AsioHelloServer(asio::io_context& ioc, const std::string& ip, std::uint16_t port)
      : ioc_(ioc), acceptor_(ioc) {
    asio::ip::tcp::endpoint ep(asio::ip::make_address(ip), port);
    this->acceptor_.open(ep.protocol());
    this->acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    this->acceptor_.bind(ep);
    this->acceptor_.listen(asio::socket_base::max_listen_connections);
    asio::co_spawn(this->ioc_, this->accept_loop(), asio::detached);
  }

  /// @brief port actually bound (useful when constructed with port == 0)
  [[nodiscard]] std::uint16_t port() const { return this->acceptor_.local_endpoint().port(); }

private:
  asio::awaitable<void> accept_loop() {
    for (;;) {
      asio::ip::tcp::socket socket(this->ioc_);
      co_await this->acceptor_.async_accept(socket, asio::use_awaitable);
      std::error_code ignore_ec;
      socket.set_option(asio::ip::tcp::no_delay(true), ignore_ec);
      asio::co_spawn(this->ioc_, this->session(std::move(socket)), asio::detached);
    }
  }

  asio::awaitable<void> session(asio::ip::tcp::socket socket) {
    try {
      asio::streambuf buf;
      for (;;) {
        std::size_t n = co_await asio::async_read_until(socket, buf, "\r\n\r\n",
                                                        asio::use_awaitable);
        auto data = buf.data();
        std::string head(asio::buffers_begin(data), asio::buffers_begin(data) + n);
        buf.consume(n);
        auto request = parse_request_head(head);
        if (!request) {
          co_return;
        }
        // drain the request body, the bytes may already be in the buffer
        if (request->content_length != 0) {
          if (buf.size() > 0) {
            std::size_t drop = std::min(buf.size(), request->content_length);
            buf.consume(drop);
            request->content_length -= drop;
          }
          while (request->content_length != 0) {
            std::size_t drop = std::min<std::size_t>(request->content_length, 4096);
            co_await asio::async_read(socket, buf, asio::transfer_exactly(drop),
                                      asio::use_awaitable);
            buf.consume(drop);
            request->content_length -= drop;
          }
        }
        int status = request->method == "GET" && request->target == HELLO_PATH ? 200 : 404;
        std::string response = build_http_response(status,
                                                   status == 200 ? HELLO_BODY
                                                                 : std::string_view{},
                                                   !request->keep_alive);
        co_await asio::async_write(socket, asio::buffer(response), asio::use_awaitable);
        if (!request->keep_alive) {
          co_return;
        }
      }
    } catch (const std::exception&) {
      // connection error or closed by peer, just drop it
    }
  }

  asio::io_context& ioc_;
  asio::ip::tcp::acceptor acceptor_;
};

}  // namespace http_bench
#endif
