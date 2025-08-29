/**
 * @file request.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP request parsing tests
 * @version 0.1.0
 * @date 2025-07-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <gtest/gtest.h>
#include <xsl/asio.h>

using namespace xsl::http;
using namespace xsl;
using namespace xsl::asio;
TEST(http_parse, complete) {
  Request parser;
  std::string_view data = "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  RequestLine line;
  auto [sz, res] = line.parse(data);
  ASSERT_EQ(res, errc{});
  ASSERT_EQ(line.method, Method::GET);
  ASSERT_EQ(line.path, "/");
  ASSERT_EQ(Version::from_string_view(line.version), Version::HTTP_1_1);
  MessageRestView rest;
  auto [sz_rest, res_rest] = rest.parse(data.substr(sz));
  ASSERT_EQ(res_rest, errc{});
  ASSERT_EQ(sz + sz_rest, data.size());
  ASSERT_EQ(rest.headers.size(), 1);
  ASSERT_EQ(rest.headers["Host"], "localhost:8080");
}
TEST(http_parse, empty) {
  std::string_view data = "";
  RequestLine line;
  auto [sz, res] = line.parse(data);
  ASSERT_NE(res, errc{});
  ASSERT_EQ(res, errc::resource_unavailable_try_again);
}
TEST(http_parse, partial) {
  std::string_view data = "GET / HTTP/1.1\r\nHost: localhost:8080";
  RequestLine line;
  auto [sz, res] = line.parse(data);
  ASSERT_EQ(res, errc{});
  MessageRestView rest;
  auto [sz_rest, res_rest] = rest.parse(data.substr(sz));
  ASSERT_EQ(res_rest, errc::resource_unavailable_try_again);
}
TEST(http_parse, invalid_format) {
  std::string_view data = "GET / HTTP/1.1\rHost: localhost:8080\r\n\r\n";
  RequestLine line;
  auto [sz, res] = line.parse(data);
  ASSERT_EQ(res, errc::illegal_byte_sequence);
}
TEST(http_parse, test_version) {
  std::string_view data = "GET / HTTP/1.0\r\nHost: localhost:8080\r\n\r\n";
  RequestLine line;
  auto [sz, res] = line.parse(data);
  ASSERT_EQ(res, errc{});
  ASSERT_EQ(line.method, Method::GET);
  ASSERT_EQ(line.path, "/");
  ASSERT_EQ(line.version, "HTTP/1.0");
}
TEST(http_parse, test_query) {
  std::string_view data = "GET /?a=1&b=2 HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  RequestLine line;
  auto [sz, res] = line.parse(data);
  ASSERT_EQ(res, errc{});
  ASSERT_EQ(line.method, Method::GET);
  ASSERT_EQ(line.path, "/");
  ASSERT_EQ(line.query, "a=1&b=2");
}
TEST(http_parse, test_query_empty) {
  std::string_view data = "GET /?a=1&b=2 HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  RequestLine line;
  auto [sz, res] = line.parse(data);
  ASSERT_EQ(res, errc{});
  ASSERT_EQ(line.method, Method::GET);
  ASSERT_EQ(line.path, "/");
  ASSERT_EQ(line.query, "a=1&b=2");
}
TEST(request_builder, complete) {
  RequestPartBuilder builder;
  builder.set_method(Method::GET)
      .set_target("/")
      .set_version(Version::HTTP_1_1)
      .add_header("Host", "localhost:8080");
  ASSERT_EQ(builder.to_string(), "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n");
}

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
